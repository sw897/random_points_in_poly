#include "poly2tri.h"
#include "common/triangle_utils.h"

#include "shapefil.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


int main(int argc, char ** argv)
{

    using namespace p2t;
    using namespace std;
    //random_points_in_triangle(110., 35., 120., 35., 115., 40., 10000, "out.csv");

    string adm_shp = "/download/china_adm/china_adm_1.shp";
    int		nShapeType, nEntities, i, iPart, bValidate = 1,nInvalidCount=0;
    const char 	*pszPlus;
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

        if( psShape->bMeasureIsUsed )
            printf( "\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
                    "  Bounds:(%.15g,%.15g, %.15g, %.15g)\n"
                    "      to (%.15g,%.15g, %.15g, %.15g)\n",
                    i, SHPTypeName(psShape->nSHPType),
                    psShape->nVertices, psShape->nParts,
                    psShape->dfXMin, psShape->dfYMin,
                    psShape->dfZMin, psShape->dfMMin,
                    psShape->dfXMax, psShape->dfYMax,
                    psShape->dfZMax, psShape->dfMMax );
        else
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

        //
        vector<vector<Point*>> polylines;
        // redirect cout
        stringstream sstr;
        sstr << i;
        string out_file = "tri_vertex_" + sstr.str() + ".csv";
        ofstream output(out_file);
        streambuf* fileBuf = output.rdbuf();
        streambuf* coutBuf = cout.rdbuf();
        cout.rdbuf(fileBuf);

        for ( iPart = 0; iPart < psShape->nParts; iPart++)
        {
            vector<Point*> polyline;
            int start = psShape->panPartStart[iPart];
            int end = psShape->nVertices;
            if (iPart < psShape->nParts - 1)
                end = psShape->panPartStart[iPart + 1];

            // remove the last point, because it is the same with the first one
            for( j = start; j < end - 1; j++ )
            {
                const char	*pszPartType = "";

                if( j == 0 && psShape->nParts > 0 )
                    pszPartType = SHPPartTypeName( psShape->panPartType[0] );

                if (polyline.size() > 0)
                {
                    Point& pt = *polyline[polyline.size()-1];
                    if (pt.x == psShape->padfX[j] && pt.y == psShape->padfY[j])
                        continue;
                }
                polyline.push_back(new Point(psShape->padfX[j], psShape->padfY[j]));
            }
            polylines.push_back(polyline);

            // trianglate
            CDT* cdt = new CDT(polyline);
            cdt->Triangulate();
            vector<Triangle*> triangles = cdt->GetTriangles();
            list<Triangle*> map = cdt->GetMap();

            // print
            int icount = 0;
            for (auto it = triangles.begin(); it != triangles.end(); ++it)
            {
                Triangle& tri = **it;
                for (int iVertex = 0; iVertex < 3; ++iVertex)
                {
                    Point& pt = *tri.GetPoint(iVertex);
                    cout << pt.x << "," << pt.y << endl;
                    printf("%d, ", icount*3 + iVertex);
                }
                icount++;
            }
        }
        if( bValidate )
        {
            int nAltered = SHPRewindObject( hSHP, psShape );

            if( nAltered > 0 )
            {
                printf( "  %d rings wound in the wrong direction.\n",
                        nAltered );
                nInvalidCount++;
            }
        }

        // free
        for (auto it = polylines.begin(); it != polylines.end(); ++it)
        {
            for (auto it2 = it->begin(); it2 != it->end(); ++it2)
            {
                delete *it2;
            }
        }

        // resume cout
        output.flush();
        output.close();
        std::cout.rdbuf(coutBuf);

        SHPDestroyObject( psShape );
        break;
    }

    SHPClose( hSHP );

    if( bValidate )
    {
        printf( "%d object has invalid ring orderings.\n", nInvalidCount );
    }

    return 0;
}
