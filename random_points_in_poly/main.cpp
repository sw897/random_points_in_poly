#include "poly2tri.h"
#include "common/triangle_utils.h"
#include "clipper.hpp"
#include "shapefil.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const double COORDINATE_PRECISION = 10000000000000.;

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

int simple_polygon(string shp_file, int id)
{
    int		nShapeType, nEntities;
    double 	adfMinBound[4], adfMaxBound[4];

    SHPHandle	hSHP = SHPOpen( shp_file.c_str(), "r+b" );

    if( hSHP == NULL )
    {
        printf( "Unable to open:%s\n", shp_file.c_str() );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    SHPGetInfo( hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound );

    SHPObject	*psShape = SHPReadObject( hSHP, id );

    if( psShape == NULL )
    {
        fprintf( stderr,
                 "Unable to read shape %d, terminating object reading.\n",
                id );
        return 0;
    }

    int new_nVertices = 0;
    Paths all_paths;
    for (int iPart = 0; iPart < psShape->nParts; iPart++)
    {
        // 1. simplify polygon using clipper
        Path path;
        Paths out_paths;
        int start = psShape->panPartStart[iPart];
        int end = psShape->nVertices;
        if (iPart < psShape->nParts - 1)
            end = psShape->panPartStart[iPart + 1];

        for (int j = start; j < end; j++)
        {
            cInt x = cInt(psShape->padfX[j]*COORDINATE_PRECISION);
            cInt y = cInt(psShape->padfY[j]*COORDINATE_PRECISION);
            path.push_back(IntPoint(x, y));
        }
        SimplifyPolygon(path, out_paths);
        if(out_paths.size() > 1)
        {

            for(auto it = out_paths.begin(); it != out_paths.end(); ++it)
            {
                all_paths.push_back(*it);
                new_nVertices += it->size();
            }
        }
        else
        {
            all_paths.push_back(path);
            new_nVertices += path.size();
        }
    }

    int nPanPartType = psShape->panPartType[0];
    int new_nParts = all_paths.size();
    int * new_panPartStart = (int *) malloc (sizeof(int)*new_nParts);
    int * new_panPartType = (int *) malloc (sizeof(int)*new_nParts);
    double * new_padfX = (double *) malloc (sizeof(double)*new_nVertices);
    double * new_padfY = (double *) malloc (sizeof(double)*new_nVertices);

    new_panPartStart[0] = 0;
    new_panPartType[0] = 0;
    for(int i = 0; i < new_nParts-1; ++i)
    {
        new_panPartType[i+1] = nPanPartType;
        new_panPartStart[i+1] = new_panPartStart[i] + all_paths[i].size();
    }
    for (int i = 0; i < new_nParts; ++i)
    {
        for (int j = 0; j < all_paths[i].size(); ++j)
        {
            new_padfX[new_panPartStart[i] + j] = double(all_paths[i][j].X)/COORDINATE_PRECISION;
            new_padfY[new_panPartStart[i] + j] = double(all_paths[i][j].Y)/COORDINATE_PRECISION;
        }
    }

    SHPObject *newShape = SHPCreateObject( nShapeType, id, new_nParts,
                                           new_panPartStart, new_panPartType,
                                           new_nVertices,
                                           new_padfX, new_padfY, NULL, NULL);
    SHPWriteObject(hSHP, id, newShape);

    // destroy shapeobj
    SHPDestroyObject( psShape );
    SHPDestroyObject( newShape );

    SHPClose( hSHP );

    free( new_panPartStart );
    free( new_panPartType );
    free( new_padfX );
    free( new_padfY );
    return 0;
}

int export_polygon_triangles(string shp_file)
{

    //random_points_in_triangle(110., 35., 120., 35., 115., 40., 10000, "out.csv");

    int		nShapeType, nEntities, i, iPart, nInvalidCount=0;
    double 	adfMinBound[4], adfMaxBound[4];

    SHPHandle	hSHP = SHPOpen( shp_file.c_str(), "rb" );

    if( hSHP == NULL )
    {
        printf( "Unable to open:%s\n", shp_file.c_str() );
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
                    //print_triangle(triangles);

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

        }

        cout << "  ]" << endl;
        cout << "}" << endl;

        printf(" total area is %lf \n", total_area);

        // resume cout
        output.flush();
        output.close();
        std::cout.rdbuf(coutBuf);

        // destroy shapeobj
        SHPDestroyObject( psShape );
    }

    SHPClose( hSHP );

    return 0;
}


int main()
{
//    simple_polygon("/download/china_vector/china_adm_1.shp", 14);
//    simple_polygon("/download/china_vector/china_adm_2.shp", 144);
//    simple_polygon("/download/china_vector/china_adm_3.shp", 1033);
    simple_polygon("/download/china_vector/world_adm.shp", 131);
    cout << "process ok" << endl;
    return 0;
}
