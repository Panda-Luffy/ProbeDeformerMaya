/**
 * @file tetrise.h
 * @brief tet-rise surface mesh
 * @section LICENSE The MIT License
 * @section requirements:  Eigen library,   (optional) MKL
 * @version 0.10
 * @date  Jul. 2014
 * @author Shizuo KAJI
 */


#pragma once

// uncomment if you use MKL
// #define  EIGEN_USE_MKL_ALL

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <Eigen/Geometry>

#include <cmath>
#include <iostream>
#include <cassert>
#include <vector>
#include <map>


// tetrahedra construction mode
#define TM_FACE 0
#define TM_EDGE 1
#define TM_VERTEX 2
#define TM_VFACE 10   // for each vertex, gathering faces+normal


/// threshold for being zero
#define EPSILON 10e-6

using namespace Eigen;

// class for a pair of numerics
template<class T>
class couple{
public:
    T left, right;
    couple() {};
    couple(T l, T r){
        left = l;
        right = r;
    }
    bool operator==(const couple<T>& pa){
        return (left == pa.left && right == pa.right);
    }
    bool operator<(const couple<T>& pa) const {
        return ( left<pa.left || (left==pa.left && right<pa.right));
    }
};

// edge data
class edge{
public:
    std::vector<int> vertices;
    std::vector<int> faces;
    edge(){
        vertices.resize(2);
        faces.resize(2);
    }
    edge(int s,int t,int f,int g){
        vertices.resize(2);
        faces.resize(2);
        vertices[0]=s;   // vertex index of Maya
        vertices[1]=t;
        faces[0]=f;   // adjacent face index of faceList
        faces[1]=g;
    }
    edge &operator=(const edge &e){
        vertices[0]=e.vertices[0];
        vertices[1]=e.vertices[1];
        faces[0]=e.faces[0];
        faces[1]=e.faces[1];
        return(*this);
    }
};

// vertex data
class vertex{
public:
    int index;  // vertex index of Maya
    std::vector<int> connectedTriangles; // list of vertex indices. sorted in such a way that index-1-2, index-3-4,..., index-(last-1)-last form oriented faces
    vertex(){
        connectedTriangles.clear();
    }
    vertex(int idx, std::vector<int> list){
        index = idx;
        connectedTriangles=list;
    }
    vertex &operator=(const vertex &v){
        index = v.index;
        connectedTriangles=v.connectedTriangles;
        return(*this);
    }
};



///
namespace Tetrise{
    // compose a matrix out of four vectors
    Matrix4d mat(Vector3d p0,Vector3d p1,Vector3d p2,Vector3d c){
        Matrix4d m;
        m << p0[0], p0[1], p0[2], 1,
        p1[0], p1[1], p1[2], 1,
        p2[0], p2[1], p2[2], 1,
        c[0], c[1], c[2], 1;
        return m;
    }
    
    // make the list of (inner) edges
    int makeEdgeList(const std::vector<int>& faceList, std::vector<edge>& edgeList){
        edgeList.clear();
        edgeList.reserve(faceList.size());
        std::map< couple<int>, int > edges;
        int s,t;
        for(int i=0;i<faceList.size()/3;i++){
            for(int j=0;j<3;j++){
                s=faceList[3*i+j];
                t=faceList[3*i+((j+1)%3)];
                if(s>t) swap(s,t);
                couple<int> pa(s,t);
                if( edges.find(pa) == edges.end() ){  // if not in the list
                    edges[pa] = i;
                }else{
                    edge newEdge(s,t,edges[pa],i);
                    edgeList.push_back(newEdge);
                }
            }
        }
        return (int)edgeList.size();
    }
    
    // make the list of tetrahedra
    int makeTetList(short tetMode, int numPts, const std::vector<int>& faceList,
                    const std::vector<edge>& edgeList, const std::vector<vertex>& vertexList,
                    std::vector<int>& tetList){
        tetList.clear();
        int dim=0;    // number of total points including ghost ones
        if(tetMode == TM_FACE){
            int numTet = (int)faceList.size()/3;
            dim = numTet + numPts;
            tetList.resize(4*numTet);
            for(int i=0;i<numTet;i++){
                tetList[4*i] = faceList[3*i];
                tetList[4*i+1] = faceList[3*i+1];
                tetList[4*i+2] = faceList[3*i+2];
                tetList[4*i+3] = i+numPts;
            }
        }else if(tetMode == TM_EDGE){
            int numTet = 2 * (int)edgeList.size();
            dim = numPts + (int)edgeList.size();
            tetList.resize(4* numTet);
            for(int i=0;i<edgeList.size();i++){
                for(int j=0;j<2;j++){
                    int f=edgeList[i].faces[j];
                    int k=0;
                    while(faceList[3*f+k]==edgeList[i].vertices[0]  // first two vertices should be the edge
                          || faceList[3*f+k]==edgeList[i].vertices[1]){
                        k++;
                    }
                    assert(k<3);
                    tetList[8*i + 4*j]=faceList[3*f + ((k+1)%3)];
                    tetList[8*i + 4*j+1]=faceList[3*f + ((k+2)%3)];
                    tetList[8*i + 4*j+2]=faceList[3*f + k];
                    tetList[8*i + 4*j+3]=i+numPts;
                }
            }
        }else if(tetMode == TM_VERTEX){
            int numTet = (int)faceList.size();
            tetList.reserve(numTet);
            dim = numPts + (int)vertexList.size();
            for(int i=0;i<vertexList.size();i++){
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    tetList.push_back(vertexList[i].index);   // the first vertex should be the vertex
                    tetList.push_back(vertexList[i].connectedTriangles[2*j]);
                    tetList.push_back(vertexList[i].connectedTriangles[2*j+1]);
                    tetList.push_back(numPts+i);
                }
            }
        }else if(tetMode == TM_VFACE){
            int numTet = (int)faceList.size();
            tetList.reserve(numTet);
            int cur=0;
            for(int i=0;i<vertexList.size();i++){
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    tetList.push_back(vertexList[i].index);   // the first vertex should be the vertex
                    tetList.push_back(vertexList[i].connectedTriangles[2*j]);
                    tetList.push_back(vertexList[i].connectedTriangles[2*j+1]);
                    tetList.push_back(numPts+cur);
                    cur++;
                }
            }
            dim = numPts + (int) tetList.size()/4;
        }
        return dim;
    }
    
    // comptute tetrahedra weights from those of points
    void makeWeightList(short tetMode, const std::vector<int>& tetList,
                   const std::vector<int>& faceList, const std::vector<edge>& edgeList,
                   const std::vector<vertex>& vertexList, const std::vector<double>& ptsWeight,
                   std::vector<double>& tetWeight ){
        int numTet = (int)tetList.size()/4;
        tetWeight.resize(numTet);
        if(tetMode == TM_FACE){
            for(int i=0;i<numTet;i++){
                tetWeight[i] = (ptsWeight[tetList[4*i]] + ptsWeight[tetList[4*i+1]]
                                + ptsWeight[tetList[4*i+2]])/3;
            }
        }else if(tetMode == TM_EDGE){
            for(int i=0;i<edgeList.size();i++){
                tetWeight[2*i]=tetWeight[2*i+1] = (ptsWeight[edgeList[i].vertices[0]]
                                            + ptsWeight[edgeList[i].vertices[1]])/2;
            }
        }else if(tetMode == TM_VERTEX || tetMode == TM_VFACE){
            for(int i=0;i<numTet;i++){
                tetWeight[i] = ptsWeight[tetList[4*i]];
            }
        }
    }
    // comptute tetrahedra weights from those of points
    void makePtsWeightList(short tetMode, int numPts, const std::vector<int>& tetList,
                        const std::vector<int>& faceList, const std::vector<edge>& edgeList,
                        const std::vector<vertex>& vertexList, const std::vector<double>& tetWeight,
                        std::vector<double>& ptsWeight ){
        int numTet = (int)tetList.size()/4;
        ptsWeight.clear();
        ptsWeight.resize(numPts,0.0);
        if(tetMode == TM_FACE){
            for(int i=0;i<numTet;i++){
                for(int j=0;j<3;j++){
                    ptsWeight[tetList[4*i+j]] += tetWeight[i];
                }
            }
        }else if(tetMode == TM_EDGE){
            for(int i=0;i<edgeList.size();i++){
                ptsWeight[edgeList[i].vertices[0]] += tetWeight[2*i]+tetWeight[2*i+1];
                ptsWeight[edgeList[i].vertices[1]] += tetWeight[2*i]+tetWeight[2*i+1];
            }
        }else if(tetMode == TM_VERTEX || tetMode == TM_VFACE){
            for(int i=0;i<numTet;i++){
                ptsWeight[tetList[4*i]] += tetWeight[i];
            }
        }
    }


    
        // construct tetrahedra matrices
    void tetMatrix(short tetMode, const std::vector<Vector3d>& pts, const std::vector<int>& tetList,
        const std::vector<int>& faceList, const std::vector<edge>& edgeList,
                    const std::vector<vertex>& vertexList, std::vector<Matrix4d>& P){
        Vector3d u, v, q, c;
        int numTet = (int)tetList.size()/4;
        P.clear();
        if(tetMode == TM_FACE || tetMode == TM_VFACE){
            P.resize(numTet);
            for(int i=0;i<numTet;i++){
                Vector3d p0=pts[tetList[4*i]];
                Vector3d p1=pts[tetList[4*i+1]];
                Vector3d p2=pts[tetList[4*i+2]];
                u=p1-p0;
                v=p2-p0;
                q=u.cross(v).normalized();
                c = q+p0;
                P[i] = mat(p0,p1,p2,c);
            }
        }else if(tetMode == TM_EDGE){
            P.resize(numTet);
            for(int i=0;i<edgeList.size();i++){
                c = Vector3d::Zero();
                for(int j=0;j<2;j++){
                    int f=edgeList[i].faces[j];
                    u=pts[faceList[3*f+1]]-pts[faceList[3*f]];
                    v=pts[faceList[3*f+2]]-pts[faceList[3*f]];
                    q=u.cross(v).normalized();
                    c += q;
                }
                c = (pts[edgeList[i].vertices[0]]+pts[edgeList[i].vertices[1]])/2 + c.normalized();
                for(int j=0;j<2;j++){
                    Vector3d p0=pts[tetList[8*i + 4*j]];
                    Vector3d p1=pts[tetList[8*i + 4*j + 1]];
                    Vector3d p2=pts[tetList[8*i + 4*j + 2]];
                    P[2*i+j] = mat(p0,p1,p2,c);
                }
            }
        }else if(tetMode == TM_VERTEX){
            P.reserve(numTet);
            for(int i=0;i<vertexList.size();i++){
                Vector3d p0 = pts[vertexList[i].index];
                Vector3d p1,p2,c=Vector3d::Zero();
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    p1 = pts[vertexList[i].connectedTriangles[2*j]];
                    p2 = pts[vertexList[i].connectedTriangles[2*j+1]];
                    c += ((p1-p0).cross(p2-p0)).normalized();
                }
                c = p0 + c.normalized();
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    p1 = pts[vertexList[i].connectedTriangles[2*j]];
                    p2 = pts[vertexList[i].connectedTriangles[2*j+1]];
                    P.push_back( mat(p0,p1,p2,c));
                }
            }
        }
    }
    
    // construct tetrahedra matrices with normalised edges ( for use with cage )
    void tetMatrixNormalised(short tetMode, const std::vector<Vector3d>& pts, const std::vector<int>& tetList,
                   const std::vector<int>& faceList, const std::vector<edge>& edgeList,
                   const std::vector<vertex>& vertexList, std::vector<Matrix4d>& P){
        Vector3d u, v, q, c;
        int numTet = (int)tetList.size()/4;
        P.clear();
        if( tetMode == TM_FACE || tetMode == TM_EDGE){
            tetMatrix(tetMode, pts, tetList, faceList, edgeList, vertexList, P);
        }else if(tetMode == TM_VERTEX){
            P.reserve(numTet);
            for(int i=0;i<vertexList.size();i++){
                Vector3d p0 = pts[vertexList[i].index];
                Vector3d p1,p2,c=Vector3d::Zero();
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    p1 = pts[vertexList[i].connectedTriangles[2*j]];
                    p2 = pts[vertexList[i].connectedTriangles[2*j+1]];
                    c += ((p1-p0).cross(p2-p0)).normalized();
                }
                c = p0 + c.normalized();
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    p1 = p0+(pts[vertexList[i].connectedTriangles[2*j]]-p0).normalized();
                    p2 = p0+(pts[vertexList[i].connectedTriangles[2*j+1]]-p0).normalized();
                    P.push_back( mat(p0,p1,p2,c));
                }
            }
        }else if(tetMode == TM_VFACE){
            P.resize(numTet);
            for(int i=0;i<numTet;i++){
                Vector3d p0=pts[tetList[4*i]];
                Vector3d p1=p0+(pts[tetList[4*i+1]]-p0).normalized();
                Vector3d p2=p0+(pts[tetList[4*i+2]]-p0).normalized();
                u=p1-p0;
                v=p2-p0;
                q=u.cross(v).normalized();
                c = q+p0;
                P[i] = mat(p0,p1,p2,c);
            }
        }
    }

    
    // make tetrahedra adjacency list
    void makeAdjacencyList(short tetMode, const std::vector<int>& tetList,
            const std::vector<edge>& edgeList, const std::vector<vertex>& vertexList,
                           std::vector< std::vector<int> >& adjacencyList){
        adjacencyList.clear();
        adjacencyList.resize(tetList.size()/4);
        for(int i=0;i<adjacencyList.size();i++){
            adjacencyList[i].clear();
        }
        if(tetMode == TM_FACE){
            for(int i=0;i<edgeList.size();i++){
                adjacencyList[edgeList[i].faces[0]].push_back(edgeList[i].faces[1]);
                adjacencyList[edgeList[i].faces[1]].push_back(edgeList[i].faces[0]);
            }
        }else if(tetMode == TM_EDGE){
            std::vector< std::vector<int> > faceShareList(2*edgeList.size());
            for(int i=0;i<2*edgeList.size();i++){
                faceShareList[i].clear();
            }
            for(int i=0;i<edgeList.size();i++){
                adjacencyList[2*i].push_back(2*i+1);
                adjacencyList[2*i+1].push_back(2*i);
                for(int j=0;j<faceShareList[edgeList[i].faces[0]].size();j++){
                    adjacencyList[2*i].push_back(faceShareList[edgeList[i].faces[0]][j]);
                    adjacencyList[faceShareList[edgeList[i].faces[0]][j]].push_back(2*i);
                }
                for(int j=0;j<faceShareList[edgeList[i].faces[1]].size();j++){
                    adjacencyList[2*i+1].push_back(faceShareList[edgeList[i].faces[1]][j]);
                    adjacencyList[faceShareList[edgeList[i].faces[1]][j]].push_back(2*i+1);
                }
                faceShareList[edgeList[i].faces[0]].push_back(2*i);
                faceShareList[edgeList[i].faces[1]].push_back(2*i+1);
            }
        }else if(tetMode == TM_VERTEX || tetMode == TM_VFACE){
            std::map< couple<int>, int > edges;
            int s,t,cur=0;
            for(int i=0;i<vertexList.size();i++){
                std::vector<int> adj(vertexList[i].connectedTriangles.size()/2);
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    adj[j] = cur+j;
                }
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    adjacencyList[cur].insert(adjacencyList[cur].end(), adj.begin(), adj.end());
                    // list of shared edges
                    s=vertexList[i].connectedTriangles[2*j];
                    t=vertexList[i].connectedTriangles[2*j+1];
                    couple<int> pa1(vertexList[i].index,s),pa2(t,vertexList[i].index);
                    if( edges.find(pa1) == edges.end() ){  // if not in the list
                        edges[pa1] = cur;
                    }else{
                        adjacencyList[cur].push_back(edges[pa1]);
                        adjacencyList[edges[pa1]].push_back(cur);
                    }
                    if( edges.find(pa2) == edges.end() ){  // if not in the list
                        edges[pa2] = cur;
                    }else{
                        adjacencyList[cur].push_back(edges[pa2]);
                        adjacencyList[edges[pa2]].push_back(cur);
                    }
                    cur++;
                }
            }
        }
    }

    // get rid of degenerate tetrahedra
    int removeDegenerate(short tetMode, int numPts,
           std::vector<int>& tetList,  std::vector<int>& faceList, std::vector<edge>& edgeList,
                         std::vector<vertex>& vertexList, const std::vector<Matrix4d>& P){
        if (tetMode == TM_FACE){
            std::vector<int> goodList(0);
            std::vector<int> oldFaceList = faceList;
            int numTet = (int)tetList.size()/4;
            for(int i=0;i<numTet;i++){
                if( abs(P[i].determinant())>EPSILON){
                    goodList.push_back(i);
                }
            }
            int numFaces = (int)goodList.size();
            faceList.resize(3*numFaces);
            for(int i=0;i<numFaces;i++){
                faceList[3*i] = oldFaceList[3*goodList[i]];
                faceList[3*i+1] = oldFaceList[3*goodList[i]+1];
                faceList[3*i+2] = oldFaceList[3*goodList[i]+2];
            }
            makeEdgeList(faceList, edgeList);
        }else if( tetMode == TM_EDGE){
            // deep copy edgeList
            std::vector<edge> oldEdgeList(edgeList.size());
            std::copy(edgeList.begin(), edgeList.end(), oldEdgeList.begin() );
            // enumerate good edges
            std::vector<int> goodList(0);
            int numEdges = (int)edgeList.size();
            for(int i=0;i<numEdges;i++){
                if( abs(P[2*i].determinant())>EPSILON && abs(P[2*i+1].determinant())>EPSILON){
                    goodList.push_back(i);
                }
            }
            numEdges = (int)goodList.size();
            edgeList.resize(numEdges);
            for(int i=0;i<numEdges;i++){
                edgeList[i]=oldEdgeList[goodList[i]];
            }
        }else if( tetMode == TM_VERTEX || tetMode == TM_VFACE){
            // deep copy edgeList
            std::vector<vertex> oldVertexList(vertexList.size());
            std::copy(vertexList.begin(), vertexList.end(), oldVertexList.begin());
            // enumerate good edges
            std::vector<int> goodList(0);
            int cur = 0;
            for(int i=0;i<vertexList.size();i++){
                bool isGood = true;
                for(int j=0;j<vertexList[i].connectedTriangles.size()/2;j++){
                    isGood = isGood && abs(P[cur].determinant())>EPSILON;
                    cur++;
                }
                if(isGood) goodList.push_back(i);
            }
            vertexList.resize(goodList.size());
            for(int i=0;i<goodList.size();i++){
                vertexList[i] = oldVertexList[goodList[i]];
            }
        }
        return makeTetList(tetMode, numPts, faceList, edgeList, vertexList, tetList);
    }
    
    // compute tet position to be used by weighting and constraint
    void makeTetCenterList(short tetMode, const std::vector<Vector3d>& pts,
                           const std::vector<int>& tetList,
                           std::vector<Vector3d>& tetCenter ){
        int numTet = (int)tetList.size()/4;
        tetCenter.resize(numTet);
        if(tetMode == TM_FACE ){
            for(int i=0;i<numTet;i++){
                tetCenter[i]=(pts[tetList[4*i]]+pts[tetList[4*i+1]]+pts[tetList[4*i+2]])/3;
            }
        }else if(tetMode == TM_EDGE){
            for(int i=0;i<numTet;i++){
                tetCenter[i]=(pts[tetList[4*i]]+pts[tetList[4*i+1]])/2;
            }
        }else if(tetMode == TM_VERTEX || tetMode == TM_VFACE){
            for(int i=0;i<numTet;i++){
                tetCenter[i]=pts[tetList[4*i]];
            }
        }
    }
    
    double distPtLin(Vector3d p,Vector3d a,Vector3d b){
        /// compute squared distance between a line segment (ab) and a point p
        double t= (a-b).dot(p-b)/(a-b).squaredNorm();
        if(t>1){
            return (a-p).squaredNorm();
        }else if(t<0){
            return (b-p).squaredNorm();
        }else{
            return (t*(a-b)-(p-b)).squaredNorm();
        }
    }
    
    double distPtTri(Vector3d p, Vector3d a, Vector3d b, Vector3d c){
        /// compute squared distance between a triangle (abc) and a point p
        /// if p is in the outer half-space, it returns HUGE_VAL
        double s[4];
        Vector3d n=(b-a).cross(c-a);
        if(n.squaredNorm()<EPSILON){
            return (p-a).squaredNorm();
        }
        double k=n.dot(a-p);
        if(k<0) return HUGE_VAL;
        s[0]=distPtLin(p,a,b);
        s[1]=distPtLin(p,b,c);
        s[2]=distPtLin(p,c,a);
        Matrix3d A;
        A << b(0)-a(0), c(0)-a(0), n(0)-a(0),
        b(1)-a(1), c(1)-a(1), n(1)-a(1),
        b(2)-a(2), c(2)-a(2), n(2)-a(2);
        Vector3d v = A.inverse()*(p-a);  // barycentric coordinate of p
        if(v(0)>0 && v(1)>0 && v(0)+v(1)<1){
            s[3]=k*k;
        }else{
            s[3] = HUGE_VAL;
        }
        return min(min(min(s[0],s[1]),s[2]),s[3]);
    }
}