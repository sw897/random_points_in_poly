#include "poly2tri.h"
#include "common/triangle_utils.h"
#include "clipper.hpp"
#include "shapefil.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const double COORDINATE_PRECISION = 10000000.;

using namespace ClipperLib;
using namespace p2t;
using namespace std;

void print_triangle(vector<Triangle*> triangles)
{
    int icount = 0;
    // print
    for (auto it = triangles.begin(); it != triangles.end(); ++it)
    {
        if (icount != 0)
            cout << "," << endl;
        cout << "    { \"type\": \"Feature\"," << endl;

        Triangle& tri = **it;
        cout << "      \"geometry\": {\"type\": \"Polygon\",\"coordinates\": [[";
        Point& pt1 = *tri.GetPoint(0);
        Point& pt2 = *tri.GetPoint(1);
        Point& pt3 = *tri.GetPoint(2);
        cout << "[" << pt1.x / COORDINATE_PRECISION << ", " << pt1.y / COORDINATE_PRECISION << "], ";
        cout << "[" << pt2.x / COORDINATE_PRECISION << ", " << pt2.y / COORDINATE_PRECISION << "], ";
        cout << "[" << pt3.x / COORDINATE_PRECISION << ", " << pt3.y / COORDINATE_PRECISION << "], ";
        cout << "[" << pt1.x / COORDINATE_PRECISION << ", " << pt1.y / COORDINATE_PRECISION << "]";
        cout << "]]}," << endl;
        cout << "      \"properties\": {\"area\": \"kaka\"}" << endl;
        icount++;

        cout << "    }";
    }
}

int main(int argc, char ** argv)
{
    //random_points_in_triangle(110., 35., 120., 35., 115., 40., 10000, "out.csv");

    string adm_shp = "/download/china_adm/china_adm_1.shp";
    int		nShapeType, nEntities, i, iPart, nInvalidCount=0;
    double 	adfMinBound[4], adfMaxBound[4];

    SHPHandle	hSHP = SHPOpen( adm_shp.c_str(), "rb" );

    if( hSHP == NULL )
    {
        printf( "Unable to open:%s\n", adm_shp.c_str() );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    SHPGetInfo( hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound );

    printf( "Shapefile Type: %s   # of Shapes: %d\n\n",
            SHPTypeName( nShapeType ), nEntities );

    printf( "File Bounds: (%.15g,%.15g,%.15g,%.15g)\n"
            "         to  (%.15g,%.15g,%.15g,%.15g)\n",
            adfMinBound[0],
            adfMinBound[1],
            adfMinBound[2],
            adfMinBound[3],
            adfMaxBound[0],
            adfMaxBound[1],
            adfMaxBound[2],
            adfMaxBound[3] );

/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ )
    {
        int		j;
        SHPObject	*psShape;

        psShape = SHPReadObject( hSHP, i );

        if( psShape == NULL )
        {
            fprintf( stderr,
                     "Unable to read shape %d, terminating object reading.\n",
                    i );
            break;
        }

        printf( "\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
                "  Bounds:(%.15g,%.15g, %.15g)\n"
                "      to (%.15g,%.15g, %.15g)\n",
                i, SHPTypeName(psShape->nSHPType),
                psShape->nVertices, psShape->nParts,
                psShape->dfXMin, psShape->dfYMin,
                psShape->dfZMin,
                psShape->dfXMax, psShape->dfYMax,
                psShape->dfZMax );

        if( psShape->nParts > 0 && psShape->panPartStart[0] != 0 )
        {
            fprintf( stderr, "panPartStart[0] = %d, not zero as expected.\n",
                     psShape->panPartStart[0] );
        }

        int nAltered = SHPRewindObject( hSHP, psShape );
        if( nAltered > 0 )
        {
            printf( "  %d rings wound in the wrong direction.\n", nAltered );
            nInvalidCount++;
        }

        // redirect cout
        stringstream sstr;
        sstr << i;
        string out_file = "tri_vertex_" + sstr.str() + ".geojson";
        ofstream output(out_file);
        streambuf* fileBuf = output.rdbuf();
        streambuf* coutBuf = cout.rdbuf();
        cout.rdbuf(fileBuf);

        cout << "{ \"type\": \"FeatureCollection\"," << endl;
        cout << "  \"features\": [" << endl;

        double total_area = 0.0;
        int icount = 0;
        for ( iPart = 0; iPart < psShape->nParts; iPart++)
        {
            // 1. simplify polygon using clipper
            Path path;
            Paths out_paths;
            int start = psShape->panPartStart[iPart];
            int end = psShape->nVertices;
            if (iPart < psShape->nParts - 1)
                end = psShape->panPartStart[iPart + 1];

            for (j = start; j < end; j++)
            {
                long long x = cInt(psShape->padfX[j]*COORDINATE_PRECISION);
                long long y = cInt(psShape->padfY[j]*COORDINATE_PRECISION);
                path.push_back(IntPoint(cInt(x), cInt(y)));
            }
            SimplifyPolygon(path, out_paths);

            // 2.trianglate polygon using poly2tri
            for (auto it = out_paths.begin(); it != out_paths.end(); ++it)
            {
                if (it->size() > 2)
                {
                    if(icount != 0)
                        cout << "," << endl;
                    else
                        cout << endl;
                    icount++;
                    vector<Point*> polyline;
                    for (auto it2 = it->begin(); it2 != it->end(); ++it2)
                    {
                        polyline.push_back(new Point(it2->X, it2->Y));
                    }
                    // trianglate
                    CDT* cdt = new CDT(polyline);
                    cdt->Triangulate();
                    vector<Triangle*> triangles = cdt->GetTriangles();
                    print_triangle(triangles);

                    // claculate area
                    for (auto it = triangles.begin(); it != triangles.end(); ++it)
                    {
                        Triangle& tri = **it;
                        double area = triangle_area(tri);
                        if (area < 0)
                        {
                            printf("area < 0, %lf \n", area);
                            area = std::abs(area);
                        }
                        total_area += area;
                    }

                    // clear
                    delete cdt;
                    for(auto it = polyline.begin(); it != polyline.end(); ++it)
                    {
                        delete *it;
                    }
                    polyline.clear();
                }
            }

//            vector<Point*> polyline;
//            if(start < end - 2)
//            {
//                // skip the last point
//                for( j = start; j < end - 1; j++ )
//                {
//                    double x = psShape->padfX[j]*COORDINATE_PRECISION;
//                    double y = psShape->padfY[j]*COORDINATE_PRECISION;
//                    polyline.push_back(new Point(x, y));
//                }
//            }

        }

        cout << "  ]" << endl;
        cout << "}" << endl;

        printf(" total area is %lf \n", total_area);

        // resume cout
        output.flush();
        output.close();
        std::cout.rdbuf(coutBuf);

        SHPDestroyObject( psShape );
    }

    SHPClose( hSHP );

    return 0;
}
