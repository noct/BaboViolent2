/*
    Copyright 2012 bitHeads inc.

    This file is part of the BaboViolent 2 source code.

    The BaboViolent 2 source code is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    The BaboViolent 2 source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with the
    BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#include <cstdint>
#include <Zeven/Zeven.h>

#ifdef WIN32
#ifndef DEDICATED_SERVER
#include <glad/glad.h>
#endif

#else
#include "LinuxHeader.h"

#ifdef __MACOSX__
#include <SDL_opengl.h>
#else
#include <glad/glad.h>
#endif
#endif

// Les constantes
#define DKO_BUMP_MAP            0x0001
#define DKO_COLOR_ARRAY         0x0002
#define DKO_DETAIL_MAP          0x0004
#define DKO_DYNAMIC_LIGHTING    0x0008
#define DKO_FORCE_WIREFRAME     0x0010
#define DKO_MULTIPASS           0x0020
#define DKO_SELFILL_MAP         0x0040
#define DKO_SPECULAR            0x0080
#define DKO_SPECULAR_MAP        0x0100
#define DKO_TEXTURE_MAP         0x0200
#define DKO_RENDER_BB           0x0400
#define DKO_RENDER_NODE         0x0800
#define DKO_RENDER_FACE         0x1000
#define DKO_CLAMP_TEXTURE       0x2000



// La petite structure pour les layers
class _typLayer
{
public:
    // La texture ogl
    unsigned int textureID;

    // La grandeur w et h de cette texture
    int w, h;
    int bpp;

    // Le scale (nb de fois r���
    float scale;

public:
    // Le constructeur
    _typLayer()
    {
        textureID = 0;
        w = 128;
        h = 128;
        bpp = 3;
        scale = 1;
    }
    virtual ~_typLayer()
    {
#ifndef DEDICATED_SERVER
        if(textureID == 0) glDeleteTextures(1, &textureID);
#endif
    }
};



class ePTexture
{
public:
    // Le nom de la texture
    char* name;

    // Ici nos layer
    _typLayer baseMap;
    _typLayer detailMap;
    _typLayer bumpMap;
    _typLayer specularMap;
    _typLayer selfIlluminationMap;

private:
    // Pour loader
    void loadMap(FILE* fic);
    void loadTexture(_typLayer* ptrLayer, FILE* fic);
    void loadString(char* string, FILE* fic);

public:
    // Constructeur / destructeur
    ePTexture();

    // On le save
    void saveIt(char* filename);
    void saveMap(_typLayer* map, char* mapName, FILE* fic);

    // Pour le loader
    void loadIt(char* filename);
};



unsigned int createTextureFromBuffer(unsigned char *Buffer, int Width, int Height, int BytePerPixel, int Filter, bool inverse);


#ifndef DEDICATED_SERVER
#include <Zeven/lib/dkt.h>
#endif


const int MAX_MODEL = 1024;

const short DKO_VERSION = 0x0002;

const short CHUNK_DKO_VERSION = 0x0000; // short
const short CHUNK_DKO_TIME_INFO = 0x0001; // short[3]

const short CHUNK_DKO_PROPERTIES = 0x0100; // ...
const short CHUNK_DKO_NAME = 0x0110; // char*
const short CHUNK_DKO_POSITION = 0x0120; // float[3]
const short CHUNK_DKO_MATRIX = 0x0130; // float[9]

const short CHUNK_DKO_MATLIST = 0x0200; // short  (nb de material)
const short CHUNK_DKO_MATNAME = 0x0210; // char*
const short CHUNK_DKO_TEX_DKT = 0x0220; // char*
const short CHUNK_DKO_AMBIENT = 0x0230; // float[4]
const short CHUNK_DKO_DIFFUSE = 0x0240; // float[4]
const short CHUNK_DKO_SPECULAR = 0x0250; // float[4]
const short CHUNK_DKO_EMISSIVE = 0x0260; // float[4]
const short CHUNK_DKO_SHININESS = 0x0270; // short
const short CHUNK_DKO_TRANSPARENCY = 0x0280; // float
const short CHUNK_DKO_TWO_SIDED = 0x0290; // char
const short CHUNK_DKO_WIRE_FRAME = 0x02A0; // char
const short CHUNK_DKO_WIRE_WIDTH = 0x02B0; // float
const short CHUNK_DKO_TEX_DIFFUSE = 0x02C0; // char*
const short CHUNK_DKO_TEX_BUMP = 0x02D0; // char*
const short CHUNK_DKO_TEX_SPECULAR = 0x02E0; // char*
const short CHUNK_DKO_TEX_SELFILL = 0x02F0; // char*

const short CHUNK_DKO_TRI_MESH = 0x0300; // ...
//  const short CHUNK_DKO_NAME = 0x0110; // char*
//  const short CHUNK_DKO_POSITION = 0x0120; // float[3]
//  const short CHUNK_DKO_MATRIX = 0x0130; // float[9]
const short CHUNK_DKO_NB_MAT_GROUP = 0x0340; // short
const short CHUNK_DKO_MAT_ID = 0x0341; // short
const short CHUNK_DKO_NB_VERTEX = 0x0342; // long
const short CHUNK_DKO_VERTEX_ARRAY = 0x0343; // float* x NbFrame
const short CHUNK_DKO_NORMAL_ARRAY = 0x0344; // float* x NbFrame
const short CHUNK_DKO_TEXCOORD_ARRAY = 0x0345; // float*
const short CHUNK_DKO_TEXCOORD_ARRAY_ANIM = 0x0346; // float* x NbFrame

const short CHUNK_DKO_DUMMY = 0x0400;
//  const short CHUNK_DKO_NAME = 0x0110; // char*
//  const short CHUNK_DKO_POSITION = 0x0120; // float[3]
//  const short CHUNK_DKO_MATRIX = 0x0130; // float[9]

const short CHUNK_DKO_END = 0x0900; // ...



const int MAX_PER_NODE = 25;
const int MAX_RECURS = 5;

class CFace
{
public:
    // ses points
    CVector point[3];

    // Normal �chaque point
    CVector normals[9];

    // This is to prevent to alocate the memory each time.
    CVector colPoint[9];

    // La d�inition du plan de la face (normal incluse)
    float a, b, c, d;

    // Son ID
    int32_t frameID;

    // Constructeur
    CFace()
    {
        frameID = 0;
        a = 0;
        b = 0;
        c = 1;
        d = 0;
    }

    // Trouver si un segment intersect le plan (juste oui ou non l�
    bool segmentToFace(const CVector &p1, const CVector &p2)
    {
        (void)p2;
        CVector intersect;
        if(findIntersect(p1, p1, intersect))// BUG: shouldn't this be p1, p2?
        {
            if(pointInFace(intersect)) return true;
        }
        return false;
    }

    // Trouver le point d'intersection d'un segment dans ce plan
    bool findIntersect(const CVector& p1, const CVector& p2, CVector& intersectPoint)
    {
        //float epsilon = 0.1f;

        float dis1 = distancePoint(p1);
        float dis2 = distancePoint(p2);

        if(/*(dis1 < -0 && dis2 > 0) || */(dis1 > 0 && dis2 < -0))
        {
            float percent = fabsf(dis1) / (fabsf(dis1) + fabsf(dis2));
            intersectPoint = p1 + (p2 - p1) * percent;

            return true;
        }

        return false;
    }

    // Pour calculer la distance d'un point �ce plan
    float distancePoint(const CVector& mpoint) const
    {
        // Trop rapide :P
        return a * mpoint[0] + b * mpoint[1] + c * mpoint[2] - d;
    }

    // Pour cr�r les normals �chaque point
    void createNormals()
    {
        int i = 0;
        for(i = 0; i < 3; ++i)
        {
            normals[i * 3 + 0] = cross(point[(i + 1) % 3] - point[i], CVector(a, b, c));
            normalize(normals[i * 3 + 0]);
            normals[i * 3 + 1] = normals[i * 3 + 0];
            normals[i * 3 + 2] = normals[i * 3 + 0];
        }

        //--- Create point normals
        for(i = 0; i < 3; ++i)
        {
            normals[(i * 3 + 8) % 9] = normals[(i * 3 + 0) % 9] + normals[(i * 3 + 7) % 9];
            normalize(normals[(i * 3 + 8) % 9]);
        }
    }

    // savoir si le point est sur la surface
    bool pointInFace(CVector& mpoint, float radius)
    {
        /*	for (int i=0;i<3;i++)
            {
                // On cr�deux vecteur avec chaque point
                CVector d = CVector::CrossProduct(point[(i+1)%3] - point[i], CVector(a,b,c));
                d.Normalize();
                d *= radius;
                CVector u = (point[i] + d) - mpoint;
                CVector v = (point[(i+1)%3] + d) - mpoint;

                // On effectu le produit crois�de �
                CVector norm = CVector::CrossProduct(u,v);

                // On calcul la distance du produit crois�au plan
                float dis = distancePoint(mpoint+norm);

                // Si on est derri�e, ha ah!! le point n'est pas dans le plan
                if (dis < 0) return false;
            }*/

            // ses points de collision avec sphere
        colPoint[0] = point[0] + normals[0] * radius;
        colPoint[1] = point[1] + normals[1] * radius;
        colPoint[2] = point[1] + normals[2] * radius;
        colPoint[3] = point[1] + normals[3] * radius;
        colPoint[4] = point[2] + normals[4] * radius;
        colPoint[5] = point[2] + normals[5] * radius;
        colPoint[6] = point[2] + normals[6] * radius;
        colPoint[7] = point[0] + normals[7] * radius;
        colPoint[8] = point[0] + normals[8] * radius;

        for(int i = 0; i < 9; ++i)
        {
            // On cr�deux vecteur avec chaque point
            CVector u = (colPoint[i]) - mpoint;
            CVector v = (colPoint[(i + 1) % 9]) - mpoint;

            // On effectu le produit crois�de �
            CVector norm = cross(u, v);

            // On calcul la distance du produit crois�au plan
            float dis = distancePoint(mpoint + norm);

            // Si on est derri�e, ha ah!! le point n'est pas dans le plan
            if(dis < 0) return false;
        }

        return true;
    }

    // savoir si le point est sur la surface
    bool pointInFace(CVector& mpoint)
    {
        for(int i = 0; i < 3; ++i)
        {
            // On cr�deux vecteur avec chaque point
            CVector u = point[i] - mpoint;
            CVector v = point[(i + 1) % 3] - mpoint;

            // On effectu le produit crois�de �
            CVector norm = cross(u, v);

            // On calcul la distance du produit crois�au plan
            float dis = distancePoint(mpoint + norm);

            // Si on est derri�e, ha ah!! le point n'est pas dans le plan
            if(dis < 0) return false;
        }

        return true;
    }
};

class COctreeNode
{
public:
    // Le centre du cube
    CVector center;

    // Sa grosseur
    float size;

    // La liste des faces quils poc�e
    int nbFace;
    CFace **faceArray;

    // Ses 8 child
    COctreeNode *child[8];

    // Si il a des enfants
    bool haveChild;

    // sa recursive
    int recurs;

    // Sa couleur
//  float color;

public:
    // Constructeur / Destructeur
    COctreeNode(CFace **mFaceArray, int mNbFace, CVector mCenter, float mSize, int mRecurs);
    virtual ~COctreeNode();

    // Pour l'intersection ak une face
    bool faceIntersection(CFace * face);

    // Pour dessiner le octree (sera jamais utilis�sauf pour le premier test)
    void render();

    // Pour trouver un point d'intersection avec un ray casting
    bool findRayIntersection(CVector &p1, CVector &p2, float &dis, CVector &intersection, CVector &normal, int &n);

    // Pour trouver un point d'intersection avec un sphere casting
    bool findSphereIntersection(CVector &p1, CVector &p2, float rayon, float &dis, CVector &intersection, CVector &normal, int &n);
};


// La fonction la plus importante de tous
//bool SegmentToBox(CVector & p1, CVector & p2, CVector & center, CVector & size, float hl);
bool SegmentToBox(const CVector& p1, const CVector& p2, const CVector& min, const CVector& max);
bool SphereToBox(const CVector& p1, const CVector& p2, const CVector& min, const CVector& max, float rayon);



struct _typPos
{
    float v[3];
};
struct _typMat
{
    float m[9];
};
struct _typDummy
{
    // Son nom
    char *name;
    _typPos *position;
    _typMat *matrix;

    _typDummy(int duration)
    {
        name = 0;
        position = new _typPos[duration];
        matrix = new _typMat[duration];
    }
    virtual ~_typDummy()
    {
        if(name) delete[] name;
        if(position) delete[] position;
        if(matrix) delete[] matrix;
    }
};


class eHierarchic
{
public:
    // Ici la liste de ses enfant
    eHierarchic* childList;

    // Un pointeur vers le suivant
    eHierarchic* next;

    // Un pointeur vers son parent (il ne peut y en avoir qu'un)
    eHierarchic* parent;

    // Si il est enable ou pas
    bool enable;

    // Sa position
    float position[3];  // Pas utile mais bon

    // sa matrice
    float matrix[9];  // Pas utile non plus mais bon

public:
    // Constructeur / Destructeur
    eHierarchic(); virtual ~eHierarchic();

    // On ajoute un enfant
    void addChild(eHierarchic* child);

    // On enlève un enfant
    void removeChild(eHierarchic* child);

    // On détach du parent
    void detach();

    // Pour le dessiner
    virtual void drawIt() {}
    void drawAll();

    // Pour les animer
    virtual void doIt() {}
    void doAll();

    // Un set enable
    void setEnable(bool value);

    // On construit la liste des faces
    int _buildFaceList(CFace *faceArray, int index);
    virtual int _buildFaceListIt(CFace *faceArray, int index) { (void)faceArray; (void)index; return 0; }
    int _buildVertexArray(float *vertexArray, int index);
    virtual int _buildVertexArrayIt(float * vertexArray, int index) { (void)vertexArray; (void)index; return 0; }
};


class CdkoMaterial
{
public:
    // Son nom
    char *matName;

    // Sa texture dkt
    ePTexture *textureMat;

    // Sinon on a les unsigned int directement ;)
    unsigned int texDiffuse;
    unsigned int texBump;
    unsigned int texSpecular;
    unsigned int texSelfIll;

    /*	const short CHUNK_DKO_TEX_DIFFUSE = 0x02C0; // char*
        const short CHUNK_DKO_TEX_BUMP = 0x02D0; // char*
        const short CHUNK_DKO_TEX_SPECULAR = 0x02E0; // char*
        const short CHUNK_DKO_TEX_SELFILL = 0x02F0; // char*
    */
    // Les couleurs
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float emissive[4];

    // Le shininess
    short shininess;

    // La transparence
    float transparency;

    // Si le mat on voit des deux côté
    bool twoSided;

    // Son wireframe
    bool wire;
    float wireSize;

public:
    // Constructeur / Destructeur
    CdkoMaterial(); virtual ~CdkoMaterial();

    // On set les pass
    void setDiffusePass();
    void setSpecularPass();
    void setSelfIllPass();
    void setDetailPass();

    // Pour loader le materiel
    int loadFromFile(FILE *ficIn, char *path);
};



class CDkoModel : public eHierarchic
{
public:
    // Son nom
    char *name;

    // Le nb de material
    short nbMat;

    // La liste des materiels
    CdkoMaterial *materialArray;

    // Son radius
    float radius;

    // Son bounding box
    float OABB[6];
    float min[3];
    float max[3];
    bool firstVertex;

    // Le array de ses faces (pour les collision)
    int nbFace;
    CFace *faceArray;

    // Son octree pour les collisions
    COctreeNode *octree;

    // L'animation courante
    short timeInfo[3];
    short currentFrame;
    float framef;

    // ses dummies
    int dumSize;
    _typDummy* dummies[64]; // On a un max de 64 dummy là

public:
    // Constructeur / Destructeur
    CDkoModel(); virtual ~CDkoModel();

    // pour loader du fichier
    int loadFromFile(FILE *ficIn, char *path);
    int loadFromFile3DS(FILE *ficIn, char *path);

    // Pour loader ses propriétées
    void loadProperties(FILE *ficIn);

    // Pour loader un dummy
    void loadDummy(FILE *ficIn);

    // On va créer sa facelist
    void buildFaceList();
    void buildVertexArray(float * vertexArray);

    // On va créer un octree à partir de la facelist
    void buildOctree();

    // Pour loader une nouvelle animation dans ce fichier
//  int addAnimationFromFile(unsigned int modelID, char* filename, char* animationName);

    // Pour les truc 3ds
//  void readMainBlockAnim(FILE* fic3DS, typChunk &previousChunk, char* filename);
};



// pour lire un chunk
short readChunk(FILE *ficIn);

// Pour lire un string
char *readString(FILE *ficIn);


// Pour la pile des push/pop
struct _typBitFieldPile
{
public:
    unsigned int bitField;
    // Un pointeur vers le pr��ent dans la pile
    _typBitFieldPile *previous;

    // Constructeur
    _typBitFieldPile(_typBitFieldPile*pile, unsigned int mBitField)
    {
        bitField = mBitField;
        previous = pile;
    }
};


// Pour tenir les chunk du 3ds
struct typChunk
{
    unsigned short int ID; // Le chunk ID
    unsigned int mlength;  // La grandeur du chunk
    unsigned int bytesRead;// Le nb de byte lu jusqu'�maintenant
};


// La class static pour contenir le tout
class CDko
{
public:
    // Pour tenir la derni�e erreur
    static char *lastErrorString;

    // Le array des models
    static CDkoModel *modelArray[MAX_MODEL];

    // Si il a ��initialis�
    static bool inited;

    // Le bitMaskCourant
    static unsigned int renderStateBitField;

    // La pile pour le bitMaskField
    static _typBitFieldPile *bitFieldPile;

    // Le ID global utilis�pour incr�enter exemple pour les octrees
    static int32_t globalFrameID;

public:
    // Pour updater l'erreur
    static void updateLastError(char *error);
};


class CDkoModel;


// On a ça pour chaque frame
struct _typMeshAtFrame
{
    float* vertexArray;
    float* normalArray;
    float* texCoordArray;

    _typMeshAtFrame()
    {
        vertexArray = 0;
        normalArray = 0;
        texCoordArray = 0;
    }
    virtual ~_typMeshAtFrame()
    {
        if(vertexArray) delete[] vertexArray;
        if(normalArray) delete[] normalArray;
        if(texCoordArray) delete[] texCoordArray;
    }
};


class _typMatGroup
{
public:
    // Le nb de vertex
    int32_t nbVertex;

    // Les array
    _typMeshAtFrame *meshAtFrame;
    //	float* vertexArray;
    //	float* texCoordArray;
    bool animatedUV;
    //	float* normalArray;
    float* colorArray; // Pour tenir le tangent space

    float* interpolatedVA;
    float* interpolatedNA;
    float* interpolatedUV;
    float* ptrVA;
    float* ptrNA;
    float* ptrUV;


    CDkoModel* parentModel;

    // Le pointeur vers son material
    CdkoMaterial *material;

    // Constructeur / Destructeur
    _typMatGroup()
    {
        //	vertexArray = 0;
        //	texCoordArray = 0;
        //	normalArray = 0;
        colorArray = 0;
        material = 0;
        interpolatedVA = 0;
        interpolatedNA = 0;
        interpolatedUV = 0;
        ptrVA = 0;
        ptrNA = 0;
        ptrUV = 0;
        animatedUV = false;
        meshAtFrame = 0;
    }

    virtual ~_typMatGroup()
    {
        //	if (vertexArray) delete [] vertexArray;
        //	if (texCoordArray) delete [] texCoordArray;
        //	if (normalArray) delete [] normalArray;
        if(colorArray) delete[] colorArray;
        //	if (material) delete material;
        if(interpolatedVA) delete[] interpolatedVA;
        if(interpolatedNA) delete[] interpolatedNA;
        if(interpolatedUV) delete[] interpolatedUV;
        if(meshAtFrame) delete[] meshAtFrame;
    }
};



class CdkoMesh : public eHierarchic
{
public:
    // Son nom
    char *name;

    // Le nb de material group
    short nbMatGroup;

    CDkoModel* parentModel;

    // Son array de material group
    _typMatGroup *matGroupArray;

public:
    // Constructeur / Destructeur
    CdkoMesh(); virtual ~CdkoMesh();

    // pour loader du fichier
    int loadFromFile(FILE *ficIn, char *path);

    // pour loader un matgroup
    int loadMatGroup(FILE *ficIn, _typMatGroup *matGroup);

    // Pour le dessiner
    void drawIt();
    void drawBumpFull(int i);

    // On construit la liste des faces
    int _buildFaceListIt(CFace *faceArray, int index);
    int _buildVertexArrayIt(float * vertexArray, int index);
};


//
// Les trucs static
//
char *CDko::lastErrorString = 0;
CDkoModel *CDko::modelArray[MAX_MODEL];
bool CDko::inited = false;
unsigned int CDko::renderStateBitField = 0;
_typBitFieldPile *CDko::bitFieldPile = 0;
int32_t CDko::globalFrameID = 0;

//
// On update la derni�e erreur
//
void CDko::updateLastError(char *error)
{
    if (lastErrorString) delete [] lastErrorString;
    lastErrorString = new char [strlen(error)+1];
    strcpy(lastErrorString, error);
}



//
// Pour ajouter une animation �cet objet �partir d'un fichier 3ds
//
int         dkoAddAnimationFromFile(unsigned int modelID, char* filename, char* animationName)
{
    (void)modelID; (void)filename; (void)animationName;
    return 0;
    /*
    if (CDko::modelArray[modelID])
    {
        return CDko::modelArray[modelID]->addAnimationFromFile(modelID, filename, animationName);
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return 0;
    }*/
}



//
// Pour ajouter une lumi�e �un model
//
void            dkoAddLight(unsigned int modelID, float *position, float *diffuse, float *specular, float range)
{
    (void)modelID; (void)position; (void)diffuse; (void)specular; (void)range;
}



//
// Pour construire l'arbre octal pour g�er rapidement les collisions avec ses faces
//
void            dkoBuildOctree(unsigned int modelID)
{
    if (CDko::modelArray[modelID])
    {
        if (!CDko::modelArray[modelID]->faceArray)
        {
            CDko::modelArray[modelID]->buildFaceList();
        }
        if (!CDko::modelArray[modelID]->octree && CDko::modelArray[modelID]->nbFace)
        {
            CDko::modelArray[modelID]->buildOctree();
        }
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}



//
// Calculer le color array pour le static lighting
//
void            dkoComputeStaticLighting(unsigned int modelID)
{
    (void)modelID;
}



//
// Enabeler un flag
//
void            dkoEnable(unsigned int renderFlags)
{
    CDko::renderStateBitField |= renderFlags;
}



//
// Pour �facer un model de la ram
//
void            dkoDeleteModel(unsigned int *modelID)
{
    if (CDko::modelArray[*modelID])
    {
        delete CDko::modelArray[*modelID];
        CDko::modelArray[*modelID] = 0;
    }

    *modelID = 0;
}



//
// Disabeler un flag
//
void            dkoDisable(unsigned int renderFlags)
{
    CDko::renderStateBitField &= ~renderFlags;
}



//
// Pour obtenir les infos sur un dummy
//
unsigned int    dkoGetDummy(char * dummyName, unsigned int modelID)
{
    if (CDko::modelArray[modelID])
    {
        for (int i=0;i<CDko::modelArray[modelID]->dumSize;i++)
        {
            _typDummy * dummy = CDko::modelArray[modelID]->dummies[i];
            if (stricmp(dummy->name, dummyName) == 0)
            {
                return i+1;
            }
        }
        CDko::updateLastError("Invalide dummy name");
        return 0;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return 0;
    }
}

void            dkoGetDummyMatrix(char * dummyName, unsigned int modelID, float * mat)
{
    if (CDko::modelArray[modelID])
    {
        for (int i=0;i<CDko::modelArray[modelID]->dumSize;i++)
        {
            _typDummy * dummy = CDko::modelArray[modelID]->dummies[i];
            if (stricmp(dummy->name, dummyName) == 0)
            {
                for (int j=0;j<9;j++) mat[j] = dummy->matrix[0].m[j];
                return;
            }
        }
        CDko::updateLastError("Invalide dummy name");
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyMatrix(unsigned int dummyID, unsigned int modelID, float * mat)
{
    if (CDko::modelArray[modelID])
    {
        if (dummyID == 0)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        dummyID--;
        if ((int)dummyID >= CDko::modelArray[modelID]->dumSize)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        _typDummy * dummy = CDko::modelArray[modelID]->dummies[dummyID];
        for (int j=0;j<9;j++) mat[j] = dummy->matrix[0].m[j];
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyMatrix(char * dummyName, unsigned int modelID, float * mat, short frameID)
{
    if (CDko::modelArray[modelID])
    {
        if (frameID < 0) frameID = 0;
        if (frameID >= CDko::modelArray[modelID]->timeInfo[2]) frameID = frameID % (CDko::modelArray[modelID]->timeInfo[2]);
        for (int i=0;i<CDko::modelArray[modelID]->dumSize;i++)
        {
            _typDummy * dummy = CDko::modelArray[modelID]->dummies[i];
            if (stricmp(dummy->name, dummyName) == 0)
            {
                for (int j=0;j<9;j++) mat[j] = dummy->matrix[frameID].m[j];
                return;
            }
        }
        CDko::updateLastError("Invalide dummy name");
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyMatrix(unsigned int dummyID, unsigned int modelID, float * mat, short frameID)
{
    if (CDko::modelArray[modelID])
    {
        if (frameID < 0) frameID = 0;
        if (frameID >= CDko::modelArray[modelID]->timeInfo[2]) frameID = frameID % (CDko::modelArray[modelID]->timeInfo[2]);
        if (dummyID == 0)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        dummyID--;
        if ((int)dummyID >= CDko::modelArray[modelID]->dumSize)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        _typDummy * dummy = CDko::modelArray[modelID]->dummies[dummyID];
        for (int j=0;j<9;j++) mat[j] = dummy->matrix[frameID].m[j];
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyPosition(char * dummyName, unsigned int modelID, float * pos)
{
    if (CDko::modelArray[modelID])
    {
        if (CDko::modelArray[modelID]->dumSize == 0)
        {
            CDko::updateLastError("There is no dummy");
            return;
        }
        for (int i=0;i<CDko::modelArray[modelID]->dumSize;i++)
        {
            _typDummy * dummy = CDko::modelArray[modelID]->dummies[i];
            if (stricmp(dummy->name, dummyName) == 0)
            {
                for (int j=0;j<3;j++) pos[j] = dummy->position[0].v[j];
                return;
            }
        }
        CDko::updateLastError("Invalide dummy name");
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyPosition(unsigned int dummyID, unsigned int modelID, float * pos)
{
    if (CDko::modelArray[modelID])
    {
        if (dummyID == 0)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        dummyID--;
        if ((int)dummyID >= CDko::modelArray[modelID]->dumSize)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        _typDummy * dummy = CDko::modelArray[modelID]->dummies[dummyID];
        for (int j=0;j<3;j++) pos[j] = dummy->position[0].v[j];
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyPosition(char * dummyName, unsigned int modelID, float * pos, short frameID)
{
    if (CDko::modelArray[modelID])
    {
        if (frameID < 0) frameID = 0;
        if (frameID >= CDko::modelArray[modelID]->timeInfo[2]) frameID = frameID % (CDko::modelArray[modelID]->timeInfo[2]);
        for (int i=0;i<CDko::modelArray[modelID]->dumSize;i++)
        {
            _typDummy * dummy = CDko::modelArray[modelID]->dummies[i];
            if (stricmp(dummy->name, dummyName) == 0)
            {
                for (int j=0;j<3;j++) pos[j] = dummy->position[frameID].v[j];
                return;
            }
        }
        CDko::updateLastError("Invalide dummy name");
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}

void            dkoGetDummyPosition(unsigned int dummyID, unsigned int modelID, float * pos, short frameID)
{
    if (CDko::modelArray[modelID])
    {
        if (frameID < 0) frameID = 0;
        if (frameID >= CDko::modelArray[modelID]->timeInfo[2]) frameID = frameID % (CDko::modelArray[modelID]->timeInfo[2]);
        if (dummyID == 0)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        dummyID--;
        if ((int)dummyID >= CDko::modelArray[modelID]->dumSize)
        {
            CDko::updateLastError("Invalide dummy ID");
            return;
        }
        _typDummy * dummy = CDko::modelArray[modelID]->dummies[dummyID];
        for (int j=0;j<3;j++) pos[j] = dummy->position[frameID].v[j];
        return;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}



//
// Obtenir le nom d'un dummy (Permet aussi de savoir si ce dummy existe
//
char*           dkoGetDummyName(unsigned int dummyID, unsigned int modelID)
{
    if (CDko::modelArray[modelID])
    {
        if ((int)dummyID >= CDko::modelArray[modelID]->dumSize)
        {
            CDko::updateLastError("Invalide dummy ID");
            return 0;
        }
        _typDummy * dummy = CDko::modelArray[modelID]->dummies[dummyID];
        return dummy->name;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return 0;
    }
}



//
// Obtenir la derni�e �reur
//
char*           dkoGetLastError()
{
    return CDko::lastErrorString;
}



//
// Pour obtenir le nb de vertex total sur un model
//
int             dkoGetNbVertex(unsigned int modelID)
{
    if (CDko::modelArray[modelID])
    {
        if (!CDko::modelArray[modelID]->faceArray)
        {
            CDko::modelArray[modelID]->buildFaceList();
        }

        return CDko::modelArray[modelID]->nbFace * 3;
    }

    return 0;
}



//
// On obtiens le object aligned bounding box du model
//
void            dkoGetOABB(unsigned int modelID, float *OABB)
{
    if (CDko::modelArray[modelID])
    {
        for (int i=0;i<6;i++) OABB[i] = CDko::modelArray[modelID]->OABB[i];
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return;
    }
}



//
// On obtien le radius du model
//
float           dkoGetRadius(unsigned int modelID)
{
    if (CDko::modelArray[modelID])
    {
        return CDko::modelArray[modelID]->radius;
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return -1;
    }
}



//
// Obtenir le nb de frame dans une animation
//
short           dkoGetTotalFrame(unsigned int modelID)
{
    if (CDko::modelArray[modelID])
    {
        return CDko::modelArray[modelID]->timeInfo[2];
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return -1;
    }
}



//
// Pour obtenir le mesh interpol��un certain frame
//
float*          dkoGetVertexArray(unsigned int modelID, float frameID, int & nbVertex, float *vertexArray)
{
//  float * vertexArray = 0;

    if (CDko::modelArray[modelID])
    {
        if (!CDko::modelArray[modelID]->faceArray)
        {
            CDko::modelArray[modelID]->buildFaceList();
        }
    }

    if (CDko::modelArray[modelID])
    {
        CDko::modelArray[modelID]->framef = frameID;
        if (CDko::modelArray[modelID]->framef > CDko::modelArray[modelID]->timeInfo[2]-1)
            CDko::modelArray[modelID]->framef = -1;
        if (CDko::modelArray[modelID]->framef <= 0)
            CDko::modelArray[modelID]->framef = -1;
        CDko::modelArray[modelID]->currentFrame = short((int)frameID % (CDko::modelArray[modelID]->timeInfo[2]));

        nbVertex = CDko::modelArray[modelID]->nbFace * 3;
    //  vertexArray = new float [nbVertex * 3];
        CDko::modelArray[modelID]->buildVertexArray(vertexArray);
    }

    return vertexArray; // L'utilisateur devra penser �l'effacer lui m�e
}



//
// Initialiser le engine dko
//
void            dkoInit()
{
    CDko::lastErrorString = 0;

    for (int i=0;i<MAX_MODEL;i++) CDko::modelArray[i] = 0;

    CDko::inited = true;

    CDko::renderStateBitField = 0;
    dkoEnable(
        DKO_TEXTURE_MAP |
        DKO_DETAIL_MAP |
        DKO_BUMP_MAP |
        DKO_SPECULAR_MAP |
        DKO_SELFILL_MAP |
        DKO_DYNAMIC_LIGHTING |
        DKO_SPECULAR |
        DKO_MULTIPASS);

    CDko::bitFieldPile = 0;
#ifndef DEDICATED_SERVER
    // Init les textures
    dktInit();
#endif
}



//
// Initialiser et effacer la liste des lumi�es d'un objet
//
void            dkoInitLightList(unsigned int modelID)
{
    (void)modelID;
}



//
// Loader un fichier .dko ou .3ds
//
unsigned int    dkoLoadFile(char* filename)
{

    FILE *ficIn = fopen(filename, "rb");

    // On check la validit�du fichier
    if (!ficIn)
    {
        CDko::updateLastError("Invalide filename");
        return 0;
    }

    // On une case dans le array vite avant tout
    unsigned int ID;
    for (ID=1;ID<MAX_MODEL;ID++)
    {
        if (!CDko::modelArray[ID]) break;
    }

    // Oups, il n'y a plus de place
    if (ID == MAX_MODEL)
    {
        CDko::updateLastError("Max number of models reached : 1024");
        fclose(ficIn);
        return 0;
    }

    // Parfait, on cr�un nouveau Model dko ici
    CDko::modelArray[ID] = new CDkoModel();

    // On efface le string jusqu'au /
    size_t len = strlen(filename);
    size_t i;
    for (i = len;filename[i]!='/' && filename[i]!='\\' && i>0;i--);
    i++;
    char *path = new char [i+1];
    strncpy(path, filename, i);
    path[i] = '\0';

    // On le load du fichier
    bool _3dsFile = (stricmp(&(filename[strlen(filename)-4]), "3ds") == 0);
    if (_3dsFile)
    {
        if (!CDko::modelArray[ID]->loadFromFile3DS(ficIn, path))
        {
            delete [] path;
            fclose(ficIn);
            return 0;
        }
    }
    else
    {
        if (!CDko::modelArray[ID]->loadFromFile(ficIn, path))
        {
            delete [] path;
            fclose(ficIn);
            return 0;
        }
    }

    // On efface les cochoneries
    delete [] path;

    // On a fini dle loader, on close et retourne son ID
    fclose(ficIn);
    return ID;
}



//
// Instancier un model �partir d'un autre
//
unsigned int    dkoLoadFile(unsigned int modelID)
{
    return modelID;
}



//
// Pour savoir ce que contient un model DKO
//
void            dkoOutputDebugInfo(unsigned int modelID, char *filename)
{
    if (CDko::modelArray[modelID])
    {
        FILE *ficout = fopen(filename, "w");
        if (!ficout) return;

        for (eHierarchic* ptrObject = CDko::modelArray[modelID]->childList; ptrObject; ptrObject = ptrObject->next)
        {
            CdkoMesh * mesh = (CdkoMesh*)ptrObject;
            fprintf(ficout, "Mesh  : %s\n", mesh->name);
        }

        for (int i=0;i<CDko::modelArray[modelID]->dumSize;i++)
        {
            fprintf(ficout, "Dummy : %s\n", CDko::modelArray[modelID]->dummies[i]->name);
        }

        fclose(ficout);
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
    }
}



//
// Poper le bitfield
//
void            dkoPopRenderState()
{
    if (CDko::bitFieldPile)
    {
        _typBitFieldPile* old = CDko::bitFieldPile;
        CDko::renderStateBitField = old->bitField;
        CDko::bitFieldPile = old->previous;
        delete old;
    }
}



//
// Pusher le bitfield
//
void            dkoPushRenderState()
{
    CDko::bitFieldPile = new _typBitFieldPile(CDko::bitFieldPile, CDko::renderStateBitField);
}



//
// Pour performer une d�ection d'intersection avec un ray
//
bool            dkoRayIntersection(unsigned int modelID, float *mp1, float *mp2, float *intersect, float *normal, int &n)
{
    if (CDko::modelArray[modelID])
    {
        if (CDko::modelArray[modelID]->octree)
        {
            CVector mintersection;
            CVector mnormal;
            CVector p1(mp1[0], mp1[1], mp1[2]);
            CVector p2(mp2[0], mp2[1], mp2[2]);
            float dis = distanceFast(p1, p2);

            // Voil�on est pret, on test � dans le octree
            CDko::globalFrameID++;
            if (CDko::modelArray[modelID]->octree->findRayIntersection(p1, p2, dis, mintersection, mnormal, n))
            {
                intersect[0] = mintersection[0];
                intersect[1] = mintersection[1];
                intersect[2] = mintersection[2];
                normal[0] = mnormal[0];
                normal[1] = mnormal[1];
                normal[2] = mnormal[2];
                return true;
            }
            else
                return false;
        }
        else
        {
            CDko::updateLastError("Octree not built for that model");
            return false;
        }
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return false;
    }
}



//
// Rendre un model
//
void dkoRender(unsigned int modelID)
{
#ifndef DEDICATED_SERVER
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
        glEnable(GL_RESCALE_NORMAL);
        if (CDko::modelArray[modelID])
        {
            CDko::modelArray[modelID]->framef=-1;
            CDko::modelArray[modelID]->currentFrame = 0; // Le frame par default
            CDko::modelArray[modelID]->drawAll();

            if (CDko::modelArray[modelID]->faceArray)
            {
                if (CDko::renderStateBitField & DKO_RENDER_FACE)
                {
                    glEnable(GL_CULL_FACE);

                    // On dessine sa faceList pour le fun
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_LIGHTING);
                    glPolygonMode(GL_FRONT, GL_LINE);
                    glLineWidth(1);

                    for (int i=0;i<CDko::modelArray[modelID]->nbFace;i++)
                    {
                        glColor3f(0,1,0);
                        glBegin(GL_TRIANGLES);
                            for (int j=0;j<3;j++)
                            {
                                glVertex3fv(CDko::modelArray[modelID]->faceArray[i].point[j].s);
                            }
                        glEnd();

                        glColor3f(1,0,1);
                        glBegin(GL_LINES);
                            glVertex3fv(CDko::modelArray[modelID]->faceArray[i].point[0].s);
                            glVertex3f(
                                CDko::modelArray[modelID]->faceArray[i].point[0][0] + CDko::modelArray[modelID]->faceArray[i].a*10,
                                CDko::modelArray[modelID]->faceArray[i].point[0][1] + CDko::modelArray[modelID]->faceArray[i].b*10,
                                CDko::modelArray[modelID]->faceArray[i].point[0][2] + CDko::modelArray[modelID]->faceArray[i].c*10);
                        glEnd();
                    }
                }

                // � c TELLEMENT temporaire
                if (CDko::modelArray[modelID]->octree && CDko::renderStateBitField & DKO_RENDER_NODE) CDko::modelArray[modelID]->octree->render();
            }

            // On affiche sont BB
            if (CDko::renderStateBitField & DKO_RENDER_BB)
            {
                glPushAttrib(GL_POLYGON_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
                    glDisable(GL_TEXTURE_2D);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    glDisable(GL_LIGHTING);
                    glDisable(GL_CULL_FACE);
                    glLineWidth(1);
                    glColor3f(0,1,1);
                    glPushMatrix();
                        glTranslatef(CDko::modelArray[modelID]->OABB[0], CDko::modelArray[modelID]->OABB[1], CDko::modelArray[modelID]->OABB[2]);
                        glScalef(CDko::modelArray[modelID]->OABB[3], CDko::modelArray[modelID]->OABB[4], CDko::modelArray[modelID]->OABB[5]);
                        glBegin(GL_QUADS);
                            glNormal3f(0, -1, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, -1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, -1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, -1, 1);

                            glNormal3f(1, 0, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(1, -1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(1, -1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, 1, 1);

                            glNormal3f(0, 1, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(-1, 1, 1);

                            glNormal3f(-1, 0, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(-1, -1, 1);

                            glNormal3f(0, 0, 1);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, -1, 1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, -1, 1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, 1, 1);

                            glNormal3f(0, 0, -1);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, -1, -1);
                        glEnd();
                    glPopMatrix();
                glPopAttrib();
            }
        }
    glPopAttrib();
#else
    (void)modelID;
#endif
}



//
// Dessiner le model sur un frame sp�ifique
//
void dkoRender(unsigned int modelID, unsigned short frameID)
{
#ifndef DEDICATED_SERVER
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
        glEnable(GL_RESCALE_NORMAL);
        if (CDko::modelArray[modelID])
        {
            CDko::modelArray[modelID]->framef=-1;
            CDko::modelArray[modelID]->currentFrame = frameID % (CDko::modelArray[modelID]->timeInfo[2]);
            CDko::modelArray[modelID]->drawAll();

            if (CDko::modelArray[modelID]->faceArray)
            {
                if (CDko::renderStateBitField & DKO_RENDER_FACE)
                {
                    glEnable(GL_CULL_FACE);

                    // On dessine sa faceList pour le fun
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_LIGHTING);
                    glPolygonMode(GL_FRONT, GL_LINE);
                    glLineWidth(1);

                    for (int i=0;i<CDko::modelArray[modelID]->nbFace;i++)
                    {
                        glColor3f(0,1,0);
                        glBegin(GL_TRIANGLES);
                            for (int j=0;j<3;j++)
                            {
                                glVertex3fv(CDko::modelArray[modelID]->faceArray[i].point[j].s);
                            }
                        glEnd();

                        glColor3f(1,0,1);
                        glBegin(GL_LINES);
                            glVertex3fv(CDko::modelArray[modelID]->faceArray[i].point[0].s);
                            glVertex3f(
                                CDko::modelArray[modelID]->faceArray[i].point[0][0] + CDko::modelArray[modelID]->faceArray[i].a*10,
                                CDko::modelArray[modelID]->faceArray[i].point[0][1] + CDko::modelArray[modelID]->faceArray[i].b*10,
                                CDko::modelArray[modelID]->faceArray[i].point[0][2] + CDko::modelArray[modelID]->faceArray[i].c*10);
                        glEnd();
                    }
                }

                // � c TELLEMENT temporaire
                if (CDko::modelArray[modelID]->octree && CDko::renderStateBitField & DKO_RENDER_NODE) CDko::modelArray[modelID]->octree->render();
            }

            // On affiche sont BB
            if (CDko::renderStateBitField & DKO_RENDER_BB)
            {
                glPushAttrib(GL_POLYGON_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
                    glDisable(GL_TEXTURE_2D);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    glDisable(GL_LIGHTING);
                    glDisable(GL_CULL_FACE);
                    glLineWidth(1);
                    glColor3f(0,1,1);
                    glPushMatrix();
                        glTranslatef(CDko::modelArray[modelID]->OABB[0], CDko::modelArray[modelID]->OABB[1], CDko::modelArray[modelID]->OABB[2]);
                        glScalef(CDko::modelArray[modelID]->OABB[3], CDko::modelArray[modelID]->OABB[4], CDko::modelArray[modelID]->OABB[5]);
                        glBegin(GL_QUADS);
                            glNormal3f(0, -1, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, -1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, -1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, -1, 1);

                            glNormal3f(1, 0, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(1, -1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(1, -1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, 1, 1);

                            glNormal3f(0, 1, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(-1, 1, 1);

                            glNormal3f(-1, 0, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(-1, -1, 1);

                            glNormal3f(0, 0, 1);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, -1, 1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, -1, 1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, 1, 1);

                            glNormal3f(0, 0, -1);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, -1, -1);
                        glEnd();
                    glPopMatrix();
                glPopAttrib();
            }
        }
    glPopAttrib();
#else
    (void)modelID; (void)frameID;
#endif
}

//
// Dessiner un frame interpol�
//
void dkoRender(unsigned int modelID, float frameID)
{
#ifndef DEDICATED_SERVER
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
        glEnable(GL_RESCALE_NORMAL);
        if (CDko::modelArray[modelID])
        {
            CDko::modelArray[modelID]->framef = frameID;
            if (CDko::modelArray[modelID]->framef > CDko::modelArray[modelID]->timeInfo[2]-1)
                CDko::modelArray[modelID]->framef = -1;
            if (CDko::modelArray[modelID]->framef <= 0)
                CDko::modelArray[modelID]->framef = -1;
            CDko::modelArray[modelID]->currentFrame = short((int)frameID % (CDko::modelArray[modelID]->timeInfo[2]));
            CDko::modelArray[modelID]->drawAll();

            if (CDko::modelArray[modelID]->faceArray)
            {
                if (CDko::renderStateBitField & DKO_RENDER_FACE)
                {
                    glEnable(GL_CULL_FACE);

                    // On dessine sa faceList pour le fun
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_LIGHTING);
                    glPolygonMode(GL_FRONT, GL_LINE);
                    glLineWidth(1);

                    for (int i=0;i<CDko::modelArray[modelID]->nbFace;i++)
                    {
                        glColor3f(0,1,0);
                        glBegin(GL_TRIANGLES);
                            for (int j=0;j<3;j++)
                            {
                                glVertex3fv(CDko::modelArray[modelID]->faceArray[i].point[j].s);
                            }
                        glEnd();

                        glColor3f(1,0,1);
                        glBegin(GL_LINES);
                            glVertex3fv(CDko::modelArray[modelID]->faceArray[i].point[0].s);
                            glVertex3f(
                                CDko::modelArray[modelID]->faceArray[i].point[0][0] + CDko::modelArray[modelID]->faceArray[i].a*10,
                                CDko::modelArray[modelID]->faceArray[i].point[0][1] + CDko::modelArray[modelID]->faceArray[i].b*10,
                                CDko::modelArray[modelID]->faceArray[i].point[0][2] + CDko::modelArray[modelID]->faceArray[i].c*10);
                        glEnd();
                    }
                }

                // � c TELLEMENT temporaire
                if (CDko::modelArray[modelID]->octree && CDko::renderStateBitField & DKO_RENDER_NODE) CDko::modelArray[modelID]->octree->render();
            }

            // On affiche sont BB
            if (CDko::renderStateBitField & DKO_RENDER_BB)
            {
                glPushAttrib(GL_POLYGON_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
                    glDisable(GL_TEXTURE_2D);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    glDisable(GL_LIGHTING);
                    glDisable(GL_CULL_FACE);
                    glLineWidth(1);
                    glColor3f(0,1,1);
                    glPushMatrix();
                        glTranslatef(CDko::modelArray[modelID]->OABB[0], CDko::modelArray[modelID]->OABB[1], CDko::modelArray[modelID]->OABB[2]);
                        glScalef(CDko::modelArray[modelID]->OABB[3], CDko::modelArray[modelID]->OABB[4], CDko::modelArray[modelID]->OABB[5]);
                        glBegin(GL_QUADS);
                            glNormal3f(0, -1, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, -1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, -1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, -1, 1);

                            glNormal3f(1, 0, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(1, -1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(1, -1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, 1, 1);

                            glNormal3f(0, 1, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(-1, 1, 1);

                            glNormal3f(-1, 0, 0);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(-1, -1, 1);

                            glNormal3f(0, 0, 1);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, 1, 1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, -1, 1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, -1, 1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, 1, 1);

                            glNormal3f(0, 0, -1);
                            glTexCoord2i(0, 1);
                            glVertex3i(-1, -1, -1);
                            glTexCoord2i(0, 0);
                            glVertex3i(-1, 1, -1);
                            glTexCoord2i(1, 0);
                            glVertex3i(1, 1, -1);
                            glTexCoord2i(1, 1);
                            glVertex3i(1, -1, -1);
                        glEnd();
                    glPopMatrix();
                glPopAttrib();
            }
        }
    glPopAttrib();
#else
    (void)modelID; (void)frameID;
#endif
}

//
// Fermer le engine dko
//
void dkoShutDown()
{
    if (CDko::lastErrorString) delete [] CDko::lastErrorString;

    for (int i=0;i<MAX_MODEL;i++) if (CDko::modelArray[i]) delete CDko::modelArray[i];

    _typBitFieldPile* toKill;
    for (_typBitFieldPile* ptrBitField = CDko::bitFieldPile; ptrBitField; delete toKill)
    {
        toKill = ptrBitField;
        ptrBitField = ptrBitField->previous;
    }

#ifndef DEDICATED_SERVER
    dktShutDown();
#endif
}

//
// Pour performer un sphere intersection
//
bool dkoSphereIntersection(unsigned int modelID, float *mp1, float *mp2, float rayon, float *intersect, float *normal, int &n)
{
    if (CDko::modelArray[modelID])
    {
        if (CDko::modelArray[modelID]->octree)
        {
            CVector mintersection;
            CVector mnormal;
            CVector p1(mp1[0], mp1[1], mp1[2]);
            CVector p2(mp2[0], mp2[1], mp2[2]);
            float dis = distanceFast(p1, p2);

            // Voil�on est pret, on test � dans le octree
            CDko::globalFrameID++;
            if (CDko::modelArray[modelID]->octree->findSphereIntersection(p1, p2, rayon, dis, mintersection, mnormal, n))
            {
                intersect[0] = mintersection[0];
                intersect[1] = mintersection[1];
                intersect[2] = mintersection[2];
                normal[0] = mnormal[0];
                normal[1] = mnormal[1];
                normal[2] = mnormal[2];
                return true;
            }
            else
                return false;
        }
        else
        {
            CDko::updateLastError("Octree not built for that model");
            return false;
        }
    }
    else
    {
        CDko::updateLastError("Invalide model ID");
        return false;
    }
}


//
// Constructeur / Destructeur
//
COctreeNode::COctreeNode(CFace **mFaceArray, int mNbFace, CVector mCenter, float mSize, int mRecurs)
{
    //  color = 0;

    recurs = mRecurs;

    // On init deux ou trois ti truc avant
    nbFace = 0;
    faceArray = 0;
    haveChild = false;
    int i;
    for(i = 0; i < 8; child[i++] = 0);

    if(recurs == MAX_RECURS) return;

    size = mSize;
    center = mCenter;

    // On passe les faces pis on check si elles font parti de notre cube, si oui on la rajoute
    for(i = 0; i < mNbFace; i++)
    {
        if(faceIntersection(mFaceArray[i])) nbFace++;
    }

    if(nbFace)
    {
        // On ajoute les faces trouv�
        int j = 0;
        faceArray = new CFace*[nbFace];
        for(i = 0; i < mNbFace; i++)
        {
            if(faceIntersection(mFaceArray[i]))
            {
                faceArray[j] = mFaceArray[i]; // On l'ajoute yea!
                j++;
            }
        }

        // On check si on est en dessous du nb limite
        if(nbFace > MAX_PER_NODE)
        {
            // On check pour ses child asteur
            for(i = 0; i < 8; i++)
            {
                CVector newPos;
                switch(i)
                {
                case 0: newPos = center + CVector(-size / 2, -size / 2, -size / 2); break;
                case 1: newPos = center + CVector(-size / 2, size / 2, -size / 2); break;
                case 2: newPos = center + CVector(size / 2, size / 2, -size / 2); break;
                case 3: newPos = center + CVector(size / 2, -size / 2, -size / 2); break;
                case 4: newPos = center + CVector(-size / 2, -size / 2, size / 2); break;
                case 5: newPos = center + CVector(-size / 2, size / 2, size / 2); break;
                case 6: newPos = center + CVector(size / 2, size / 2, size / 2); break;
                case 7: newPos = center + CVector(size / 2, -size / 2, size / 2); break;
                }

                child[i] = new COctreeNode(faceArray, nbFace, newPos, size / 2, recurs + 1);
                if(child[i]->nbFace == 0)
                {
                    delete child[i];
                    child[i] = 0;
                }
                else haveChild = true;
            }
        }
    }
}

COctreeNode::~COctreeNode()
{
    if(faceArray) delete faceArray;

    for(int i = 0; i < 8; i++)
    {
        // On delete r�ursivement
        if(child[i]) delete child[i];
    }
}



//
// Pour l'intersection ak une face
//
bool COctreeNode::faceIntersection(CFace * face)
{
    // On check pour chaque segment du triangle
    for(int i = 0; i < 3; i++)
    {
        /*  CVector p1 = face->point[i];
            CVector p2 = face->point[(i+1)%3];
            float hl = CVector::Distance(p1, p2) / 2;
            if (SegmentToBox(p1, p2, center, CVector(size, size, size), hl)) return true;*/
        if(SegmentToBox(face->point[i], face->point[(i + 1) % 3], center - CVector(size, size, size), center + CVector(size, size, size))) return true;
    }

    // Ensuite on check pour chaque segment (arr�es) du cube
    CVector p1 = center + CVector(-size, -size, -size);
    CVector p2 = center + CVector(-size, -size, size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }
    p1 = center + CVector(size, -size, -size);
    p2 = center + CVector(size, -size, size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }
    p1 = center + CVector(-size, size, -size);
    p2 = center + CVector(-size, size, size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }
    p1 = center + CVector(size, size, -size);
    p2 = center + CVector(size, size, size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }

    p1 = center + CVector(-size, -size, size);
    p2 = center + CVector(size, -size, size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }
    p1 = center + CVector(-size, size, -size);
    p2 = center + CVector(size, size, -size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }
    p1 = center + CVector(-size, -size, -size);
    p2 = center + CVector(size, -size, -size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }
    p1 = center + CVector(-size, size, size);
    p2 = center + CVector(size, size, size);
    if(face->segmentToFace(p1, p2))
    {
        return true;
    }

    return false;
}



//
// On va trouver le point d'intersection avec un ray tracing
//
bool COctreeNode::findRayIntersection(CVector &p1, CVector &p2, float &dis, CVector &intersection, CVector &normal, int &n)
{
    n++;

    // On test avec notre cube
    if(SegmentToBox(p1, p2, center - CVector(size, size, size), center + CVector(size, size, size)))
    {
        //  color = 1;

            // Est-ce quil a des child?
        if(haveChild)
        {
            bool found = false;
            for(int i = 0; i < 8; i++)
            {
                if(child[i])
                {
                    if(child[i]->findRayIntersection(p1, p2, dis, intersection, normal, n)) found = true;
                }
            }
            return found;
        }
        else
        {
            // On test ses faces
            bool found = false;
            for(int i = 0; i < nbFace; i++)
            {
                if(faceArray[i]->frameID != CDko::globalFrameID)
                {
                    faceArray[i]->frameID = CDko::globalFrameID;
                    n++;
                    CVector intersect;
                    if(faceArray[i]->findIntersect(p1, p2, intersect))
                    {
                        float disc = distanceFast(intersect, p1);
                        if(disc < dis)
                        {
                            if(faceArray[i]->pointInFace(intersect))
                            {
                                p2 = intersect; // � optimise pour les tests �venir
                                intersection = intersect;
                                normal = CVector(faceArray[i]->a, faceArray[i]->b, faceArray[i]->c);
                                dis = disc;
                                found = true;
                            }
                        }
                    }
                }
            }
            return found;
        }
    }

    return false;
}



//
// Pour trouver un point d'intersection avec un sphere casting
//
bool COctreeNode::findSphereIntersection(CVector &p1, CVector &p2, float rayon, float &dis, CVector &intersection, CVector &normal, int &n)
{
    n++;

    // On test avec notre cube
    if(SphereToBox(p1, p2, center - CVector(size, size, size), center + CVector(size, size, size), rayon))
    {
        //  color = 1;

            // Est-ce quil a des child?
        if(haveChild)
        {
            bool found = false;
            for(int i = 0; i < 8; i++)
            {
                if(child[i])
                {
                    if(child[i]->findSphereIntersection(p1, p2, rayon, dis, intersection, normal, n)) found = true;
                }
            }
            return found;
        }
        else
        {
            // On test ses faces
            bool found = false;
            for(int i = 0; i < nbFace; i++)
            {
                if(faceArray[i]->frameID != CDko::globalFrameID)
                {
                    faceArray[i]->frameID = CDko::globalFrameID;
                    n++;
                    CVector intersect;
                    CVector faceNormal(faceArray[i]->a, faceArray[i]->b, faceArray[i]->c);
                    if(faceArray[i]->findIntersect(
                        p1 - faceNormal * rayon,
                        p2 - faceNormal * rayon,
                        intersect))
                    {
                        float disc = distanceFast(intersect + faceNormal * rayon, p1);
                        if(disc < dis)
                        {
                            if(faceArray[i]->pointInFace(intersect, rayon))
                            {
                                p2 = intersect + faceNormal * rayon; // � optimise pour les tests �venir
                                intersection = p2;
                                normal = faceNormal;
                                dis = disc;
                                found = true;
                            }
                            /*  else
                                {
                                    // Il n'y a pas d'intersection avec l'interieur du triangle, alors on check ak ses arr�es
                                    for (int a=0;a<3;a++)
                                    {
                                        // On trouve la distance entre les deux droite
                                        CVector d1 = faceArray[i]->point[a];
                                        CVector d2 = p1;
                                        CVector u1 = faceArray[i]->point[(a+1)%3]-d1;
                                        CVector u2 = p2-p1;
                                        CVector d1d2 = d2-d1;
                                        CVector cross = CVector::CrossProduct(u1, u2);
                                        float scalar = (d1d2) * cross;
                                        float d = fabsf(scalar / CVector::Distance(CVector(0,0,0), cross));

                                        // On check si cette distance est plus petite que le rayon, sinon ce n'est pas utile de faire le test
                                        if (d <= rayon)
                                        {
                                            u1.Normalize();
                                            u2.Normalize();
                                            float scalar = u1*u2;
                                            CVector newPos = -u2 * rayon * scalar

                                            float r = rayon / scalar;
                                            CVector D = (cross * (cross * d1d2));
                                            float a = sqrtf(r*r - d*d);
                                            CVector A = -u2;
                                            A.Normalize();
                                            A *= a;
                                            CVector R = A-D;

                                            // On d�lace notre segment avec R et on trouve le point d'intersection des deux segment
                                            CVector d3 = d1 + R;
                                            CVector d3d2 = (d2 - d3);
                                            CVector u2u1 = (u1 - u2);
                                            d3d2 /= u2u1;
                                            CVector newPos = d3d2;
                                            CVector onSeg = newPos - R;

                                            // Bon, maintenant on check s'il fait parti du segment
                                            float dis1 = CVector::Distance(onSeg, d1);
                                            float dis2 = CVector::Distance(onSeg, d1+u1);
                                            float dist = CVector::Distance(d1, d1+u1);
                                            if (dis1+dis2 <= dist + d + .1f)
                                            {
                                                // COLLISIONS ENFIN CRISS!!!!!!
                                                intersection = newPos;
                                                normal = R / r;
                                                p2 = intersection;
                                                dis = disc;
                                                found = true;
                                                break;
                                            }
                                        }
                                    }
                                }*/
                        }
                    }
                }
            }
            return found;
        }
    }

    return false;
}



//
// Pour dessiner le octree (sera jamais utilis�sauf pour le premier test)
//
void COctreeNode::render()
{
#ifndef DEDICATED_SERVER
    glPushAttrib(GL_POLYGON_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glLineWidth(1);
    glColor3f(1, 1, 0);
    glPushMatrix();
    glTranslatef(center[0], center[1], center[2]);
    glScalef(size, size, size);
    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glTexCoord2i(0, 1);
    glVertex3i(-1, -1, 1);
    glTexCoord2i(0, 0);
    glVertex3i(-1, -1, -1);
    glTexCoord2i(1, 0);
    glVertex3i(1, -1, -1);
    glTexCoord2i(1, 1);
    glVertex3i(1, -1, 1);

    glNormal3f(1, 0, 0);
    glTexCoord2i(0, 1);
    glVertex3i(1, -1, 1);
    glTexCoord2i(0, 0);
    glVertex3i(1, -1, -1);
    glTexCoord2i(1, 0);
    glVertex3i(1, 1, -1);
    glTexCoord2i(1, 1);
    glVertex3i(1, 1, 1);

    glNormal3f(0, 1, 0);
    glTexCoord2i(0, 1);
    glVertex3i(1, 1, 1);
    glTexCoord2i(0, 0);
    glVertex3i(1, 1, -1);
    glTexCoord2i(1, 0);
    glVertex3i(-1, 1, -1);
    glTexCoord2i(1, 1);
    glVertex3i(-1, 1, 1);

    glNormal3f(-1, 0, 0);
    glTexCoord2i(0, 1);
    glVertex3i(-1, 1, 1);
    glTexCoord2i(0, 0);
    glVertex3i(-1, 1, -1);
    glTexCoord2i(1, 0);
    glVertex3i(-1, -1, -1);
    glTexCoord2i(1, 1);
    glVertex3i(-1, -1, 1);

    glNormal3f(0, 0, 1);
    glTexCoord2i(0, 1);
    glVertex3i(-1, 1, 1);
    glTexCoord2i(0, 0);
    glVertex3i(-1, -1, 1);
    glTexCoord2i(1, 0);
    glVertex3i(1, -1, 1);
    glTexCoord2i(1, 1);
    glVertex3i(1, 1, 1);

    glNormal3f(0, 0, -1);
    glTexCoord2i(0, 1);
    glVertex3i(-1, -1, -1);
    glTexCoord2i(0, 0);
    glVertex3i(-1, 1, -1);
    glTexCoord2i(1, 0);
    glVertex3i(1, 1, -1);
    glTexCoord2i(1, 1);
    glVertex3i(1, -1, -1);
    glEnd();
    glPopMatrix();
    glPopAttrib();
#endif

    // On dessine le cube genreeee
    if(haveChild)
    {
        for(int i = 0; i < 8; i++)
        {
            if(child[i]) child[i]->render();
        }
    }
}



//
// On check segment-to-box
//
bool SegmentToBox(const CVector& p1, const CVector& p2, const CVector& min, const CVector& max)
{
    CVector d = (p2 - p1) * 0.5f;
    CVector e = (max - min) * 0.5f;
    CVector c = p1 + d - (min + max) * 0.5f;
    CVector ad = CVector(fabsf(d[0]), fabsf(d[1]), fabsf(d[2]));; // Returns same vector with all components positive

    if(fabsf(c[0]) > e[0] + ad[0])
        return false;
    if(fabsf(c[1]) > e[1] + ad[1])
        return false;
    if(fabsf(c[2]) > e[2] + ad[2])
        return false;

    if(fabsf(d[1] * c[2] - d[2] * c[1]) > e[1] * ad[2] + e[2] * ad[1]/* + EPSILON*/)
        return false;
    if(fabsf(d[2] * c[0] - d[0] * c[2]) > e[2] * ad[0] + e[0] * ad[2]/* + EPSILON*/)
        return false;
    if(fabsf(d[0] * c[1] - d[1] * c[0]) > e[0] * ad[1] + e[1] * ad[0]/* + EPSILON*/)
        return false;

    return true;
}



//
// On check sphere-to-box
//
bool SphereToBox(const CVector& p1, const CVector& p2, const CVector& omin, const CVector& omax, float rayon)
{
    CVector min = omin - CVector(rayon, rayon, rayon);
    CVector max = omax + CVector(rayon, rayon, rayon);
    CVector d = (p2 - p1) * 0.5f;
    CVector e = (max - min) * 0.5f;
    CVector c = p1 + d - (min + max) * 0.5f;
    CVector ad = CVector(fabsf(d[0]), fabsf(d[1]), fabsf(d[2]));; // Returns same vector with all components positive

    if(fabsf(c[0]) > e[0] + ad[0])
        return false;
    if(fabsf(c[1]) > e[1] + ad[1])
        return false;
    if(fabsf(c[2]) > e[2] + ad[2])
        return false;

    if(fabsf(d[1] * c[2] - d[2] * c[1]) > e[1] * ad[2] + e[2] * ad[1]/* + EPSILON*/)
        return false;
    if(fabsf(d[2] * c[0] - d[0] * c[2]) > e[2] * ad[0] + e[0] * ad[2]/* + EPSILON*/)
        return false;
    if(fabsf(d[0] * c[1] - d[1] * c[0]) > e[0] * ad[1] + e[1] * ad[0]/* + EPSILON*/)
        return false;

    return true;
}



//
// Constructeur / Destructeur
//

CDkoModel::CDkoModel()
{
    materialArray = 0;
    nbMat = 0;
    name = 0;
    radius = 0;
    int i;
    for(i = 0; i < 6; OABB[i++] = 0.0f);
    firstVertex = true;
    nbFace = 0;
    faceArray = 0;
    octree = 0;
    timeInfo[0] = 0;
    timeInfo[1] = 0;
    timeInfo[2] = 1;
    currentFrame = 0;
    dumSize = 0;
    framef = 0;
    for(i = 0; i < 64; dummies[i++] = 0);
}

CDkoModel::~CDkoModel()
{
    if(name) delete[] name;
    if(materialArray) delete[] materialArray;
    if(octree) delete octree;

    // On efface les dummy
    for(int i = 0; i < 64; i++) if(dummies[i]) delete dummies[i];

    if(faceArray) delete[] faceArray;
}



//
// pour loader un fichier 3ds
//
int CDkoModel::loadFromFile3DS(FILE *ficIn, char *path)
{
    (void)ficIn; (void)path;
    return 0;
}



//
// pour loader du fichier
//
int CDkoModel::loadFromFile(FILE *ficIn, char *path)
{
    // On load chunk par chunk jusqu'à ce qu'on pogne le chunk End
    short chunkID = readChunk(ficIn);

    while(chunkID != CHUNK_DKO_END)
    {
        switch(chunkID)
        {
        case CHUNK_DKO_VERSION:
        {
            // On check si on a la bonne version
            short version;
            fread(&version, 1, sizeof(short), ficIn);
            if(version > DKO_VERSION)
            {
                CDko::updateLastError("Incorrect version of file");
                return 0;
            }
            break;
        }
        case CHUNK_DKO_TIME_INFO:
        {
            // On importe le start frame, le end frame et la duration dans un ti array de short
            fread(timeInfo, 1, sizeof(short) * 3, ficIn);

            // Ça va nous servir pour allouer l'espace pour nos meshes
            break;
        }
        case CHUNK_DKO_PROPERTIES:
        {
            loadProperties(ficIn);
            break;
        }
        case CHUNK_DKO_MATLIST:
        {
            fread(&nbMat, 1, sizeof(short), ficIn);
            if(nbMat)
            {
                materialArray = new CdkoMaterial[nbMat];

                // On load tout les materiaux
                for(int i = 0; i < nbMat; i++)
                {
                    if(!materialArray[nbMat - i - 1].loadFromFile(ficIn, path))
                    {
                        return 0;
                    }
                }
            }
            else
            {
                CDko::updateLastError("There is no material set in the file");
                return 0;
            }
            break;
        }
        case CHUNK_DKO_TRI_MESH:
        {
            CdkoMesh *newMesh = new CdkoMesh();
            addChild(newMesh);
            newMesh->parentModel = this;
            newMesh->loadFromFile(ficIn, path);
            break;
        }
        case CHUNK_DKO_DUMMY:
        {
            loadDummy(ficIn);
            break;
        }
        }

        chunkID = readChunk(ficIn);
    }

    // On calcul son OABB
    for(int i = 0; i < 3; i++)
    {
        OABB[0 + i] = (max[i] + min[i]) / 2;
        OABB[3 + i] = (max[i] - min[i]) / 2;
    }

    // Tout est beau
    return 1;
}



//
// Pour loader un dummy
//
void CDkoModel::loadDummy(FILE *ficIn)
{
    // On cré notre dummy
    _typDummy *newDum = new _typDummy(timeInfo[2]);

    // On load chunk par chunk jusqu'à ce qu'on pogne le chunk End
    short chunkID = readChunk(ficIn);

    while(chunkID != CHUNK_DKO_END)
    {
        switch(chunkID)
        {
        case CHUNK_DKO_NAME:
        {
            newDum->name = readString(ficIn);
            break;
        }
        case CHUNK_DKO_POSITION:
        {
            for(int i = 0; i < timeInfo[2]; i++)
            {
                fread(newDum->position[i].v, 3, sizeof(float), ficIn);
            }
            break;
        }
        case CHUNK_DKO_MATRIX:
        {
            for(int i = 0; i < timeInfo[2]; i++)
            {
                fread(newDum->matrix[i].m, 9, sizeof(float), ficIn);
            }
            break;
        }
        }

        chunkID = readChunk(ficIn);
    }

    // On l'ajoute à notre liste
    dummies[dumSize] = newDum;
    dumSize++;
}



//
// Pour loader ses propriétées
//
void CDkoModel::loadProperties(FILE *ficIn)
{
    // On load chunk par chunk jusqu'à ce qu'on pogne le chunk End
    short chunkID = readChunk(ficIn);

    while(chunkID != CHUNK_DKO_END)
    {
        switch(chunkID)
        {
        case CHUNK_DKO_NAME:
        {
            name = readString(ficIn);
            break;
        }
        case CHUNK_DKO_POSITION:
        {
            fread(position, 3, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_MATRIX:
        {
            fread(matrix, 9, sizeof(float), ficIn);
            break;
        }
        }

        chunkID = readChunk(ficIn);
    }
}



//
// Pour lire un chunk
//
short readChunk(FILE *ficIn)
{
    short chunkID;
    fread(&chunkID, 1, sizeof(short), ficIn);
    return chunkID;
}



//
// Pour lire un string
//
char *readString(FILE *ficIn)
{
    char tmp[256];
    int i = 0;
    fread(tmp, 1, 1, ficIn); // On li le premier caractère
    while(*(tmp + i++) != 0)  // Tant qu'on pogne pas le caractère 0, NULL, '\0', toute la même chose
    {
        fread(tmp + i, 1, 1, ficIn);	// On li le prochain caractère
    }

    // On cré un string dynamique et on retourne ça
    char *newStr = new char[i];
    strcpy(newStr, tmp);

    return newStr;
}



//
// On va créer sa facelist
//
void CDkoModel::buildFaceList()
{
    if(nbFace)
    {
        faceArray = new CFace[nbFace];

        // On passe tout ses child pis on construit la liste des faces pour les mesh
        _buildFaceList(faceArray, 0);
    }
}



//
// Pour se construire un vertex array
//
void CDkoModel::buildVertexArray(float * vertexArray)
{
    _buildVertexArray(vertexArray, 0);
}



//
// On va créer un octree à partir de la facelist
//
void CDkoModel::buildOctree()
{
    // On construit un array de pointeurs vers ses faces
    CFace **ptrFaceArray = new CFace*[nbFace];
    for(int i = 0; i < nbFace; i++)
        ptrFaceArray[i] = &(faceArray[i]);

    // On trouve le côté le plus grand
    float size = (OABB[3] > OABB[4]) ? OABB[3] : OABB[4];
    size = (size > OABB[5]) ? size : OABB[5];

    // On construit le octree
    CVector3f v;
    v[0] = OABB[0];
    v[1] = OABB[1];
    v[2] = OABB[2];
    octree = new COctreeNode(ptrFaceArray, nbFace, v, size, 0);

    // On efface la liste temporaire
    delete[] ptrFaceArray;
}



//
// Pour loader une nouvelle animation dans ce fichier
//
/*
int CDkoModel::addAnimationFromFile(unsigned int modelID, char* filename, char* animationName)
{
    // Pour tenir le nb de byte lu dans tout le fichier
    int byteRead = 0;

    // On ouvre le fichier en lecteur binaire
    FILE *fic3DS = fopen(filename, "rb");
    if (!fic3DS)
    {
        CDko::updateLastError("Invalide 3ds filename");
        return 0;	// Si le fichier n'a pas pus être ouvert, on retourne 0
    }

    // Ici on read le gros chunk, le premier
    typChunk currentChunk;
    ReadChunk(fic3DS, &currentChunk, filename);

    // Bon, est-ce que cest bien un fichier 3ds ça là?
    if (currentChunk.ID != M3DMAGIC)
    {
        CDko::updateLastError("Invalide 3ds file");
        fclose(fic3DS); // On ferme le fichier
        return 0; // Sinon on retourne 0 of course
    }

    // On peut maintenant processer les chunks suivant
//	dkPrintToConsole("M3DMAGIC");
    readMainBlock(fic3DS, currentChunk, filename);

    // Finalement on ferme le fichier
    fclose(fic3DS); // On ferme le fichier

    // On retourne le nombre de byte lu pendant le fichier (voir qui ça interresse :|)
    return currentChunk.mlength; // Voilà tout
}
*/


//
// Lire le block principal du fichier 3ds
//
/*
void CDkoModel::readMainBlockAnim(FILE* fic3DS, typChunk &previousChunk, char* filename)
{
    // Tant qu'on a pas lu le nb de bytes contenu dans le chunk
    while (previousChunk.bytesRead < previousChunk.mlength)
    {
        // On li maintenant le prochain chunk
        typChunk currentChunk;
        ReadChunk(fic3DS, &currentChunk, filename);

        // Check the chunk ID
        switch (currentChunk.ID)
        {
            // On load juste squi a rapport avec les animations
        case KFDATA:
            {
            //	dkPrintToConsole("KFDATA"); // Ici on va avoir un autre loader à part pour les animations
                readKFDATA(fic3DS, currentChunk, filename);
                break;
            }
        }

        // On clean ce qui reste dans le chunk
        cleanChunkByte(fic3DS, currentChunk, filename);
        previousChunk.bytesRead += currentChunk.bytesRead;
    }
}*/



//
// Pour loader nos pivot point
//
/*
void Cdko::readKFDATA(FILE* fic3DS, typChunk &previousChunk, char* filename)
{
    // Tant qu'on a pas lu le nb de bytes contenu dans le chunk
    while (previousChunk.bytesRead < previousChunk.mlength)
    {
        // On li maintenant le prochain chunk
        typChunk currentChunk;
        ReadChunk(fic3DS, &currentChunk, filename);

        // Check the chunk ID
        switch (currentChunk.ID)
        {
        case KFHDR:
            {
                dkPrintToConsole("        KFHDR");

                short revision;
                currentChunk.bytesRead += fread(&revision, 1, sizeof(short), fic3DS);

                char *strTmp = GetString(fic3DS, currentChunk, filename);
                dkPrintToConsole("            %s", strTmp);
                delete [] strTmp;

                currentChunk.bytesRead += fread(&animLen, 1, sizeof(int32_t), fic3DS);
                dkPrintToConsole("            Anim len : %i", animLen);
                break;
            }
        case KFSEG:
            {
                dkPrintToConsole("        KFSEG");

                currentChunk.bytesRead += fread(segFrame, 1, sizeof(int32_t)*2, fic3DS);
                dkPrintToConsole("            Anim segment : %i, %i", segFrame[0], segFrame[1]);
                break;
            }
        case KFCURTIME:
            {
                dkPrintToConsole("        KFCURTIME");

                currentChunk.bytesRead += fread(&curFrame, 1, sizeof(int32_t), fic3DS);
                dkPrintToConsole("            Current frame : %i", curFrame);
                break;
            }
        case OBJECT_NODE_TAG:
            {
                dkPrintToConsole("        OBJECT_NODE_TAG");
                // Ha ah! on va pogner les keyframes d'un objet
                readOBJECT_NODE_TAG(fic3DS,currentChunk,filename);
                break;
            }
        }

        // On clean ce qui reste dans le chunk
        cleanChunkByte(fic3DS, currentChunk, filename);
        previousChunk.bytesRead += currentChunk.bytesRead;
    }
}*/





//
// Constructeur / Destructeur
//
CdkoMesh::CdkoMesh()
{
    name = 0;
    nbMatGroup = 0;
    matGroupArray = 0;
}

CdkoMesh::~CdkoMesh()
{
    if(name) delete[] name;
    if(matGroupArray) delete[] matGroupArray;
}



//
// pour loader du fichier
//
int CdkoMesh::loadFromFile(FILE *ficIn, char *path)
{
    (void)path;
    // On load chunk par chunk jusqu'à ce qu'on pogne le chunk End
    short chunkID = readChunk(ficIn);

    while(chunkID != CHUNK_DKO_END)
    {
        switch(chunkID)
        {
        case CHUNK_DKO_NAME:
        {
            if(name) delete[] name;
            name = readString(ficIn);
            break;
        }
        case CHUNK_DKO_POSITION:
        {
            fread(position, 3, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_MATRIX:
        {
            fread(matrix, 9, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_NB_MAT_GROUP:
        {
            fread(&nbMatGroup, 1, sizeof(short), ficIn);

            // On load les sous-objet
            if(matGroupArray) delete[] matGroupArray;
            matGroupArray = new _typMatGroup[nbMatGroup];
            for(int i = 0; i < nbMatGroup; i++)
            {
                if(matGroupArray[i].meshAtFrame) delete[] matGroupArray[i].meshAtFrame;
                matGroupArray[i].meshAtFrame = new _typMeshAtFrame[parentModel->timeInfo[2]];
                matGroupArray[i].parentModel = parentModel;
                loadMatGroup(ficIn, &(matGroupArray[i]));
            }

            break;
        }
        }

        chunkID = readChunk(ficIn);
    }

    // Tout est beau
    return 1;
}



//
// On load un sous-objet (sous-sous objet en fait :P)
//
int CdkoMesh::loadMatGroup(FILE *ficIn, _typMatGroup *matGroup)
{
    // On load chunk par chunk jusqu'à ce qu'on pogne le chunk End
    short chunkID = readChunk(ficIn);

    while(chunkID != CHUNK_DKO_END)
    {
        switch(chunkID)
        {
        case CHUNK_DKO_MAT_ID:
        {
            short MatID;
            fread(&MatID, 1, sizeof(short), ficIn);
            matGroup->material = &(((CDkoModel*)parent)->materialArray[MatID]);
            break;
        }
        case CHUNK_DKO_NB_VERTEX:
        {
            fread(&(matGroup->nbVertex), 1, sizeof(matGroup->nbVertex), ficIn);
            printf("nbVertex: %ld\n", matGroup->nbVertex);
            break;
        }
        case CHUNK_DKO_VERTEX_ARRAY:
        {
            // On load pour tout les frames
            if(matGroup->interpolatedVA) delete[] matGroup->interpolatedVA;
            matGroup->interpolatedVA = new float[matGroup->nbVertex * 3];
            for(int f = 0; f < parentModel->timeInfo[2]; f++)
            {
                if(matGroup->meshAtFrame[f].vertexArray) delete[] matGroup->meshAtFrame[f].vertexArray;
                matGroup->meshAtFrame[f].vertexArray = new float[matGroup->nbVertex * 3];
                fread(matGroup->meshAtFrame[f].vertexArray, matGroup->nbVertex * 3, sizeof(float), ficIn);
            }

            // On pogne son parent
            CDkoModel* owner = (CDkoModel*)parent;
            owner->nbFace += matGroup->nbVertex / 2; // On incrémente son nombre de face (triangle)

            // On passe tout les vertex pour créer notre OABB
            for(int i = 0; i < matGroup->nbVertex; i++)
            {
                float tmp[3] = {
                matGroup->meshAtFrame[0].vertexArray[i * 3 + 0],
                matGroup->meshAtFrame[0].vertexArray[i * 3 + 1],
                matGroup->meshAtFrame[0].vertexArray[i * 3 + 2] };

                // On le passe dans la matrice
                CVector right(matrix[0], matrix[1], matrix[2]);
                CVector front(matrix[3], matrix[4], matrix[5]);
                CVector up(matrix[6], matrix[7], matrix[8]);
                CVector current = right * tmp[0] + front * tmp[1] + up * tmp[2] + CVector(position[0], position[1], position[2]);

                if(owner->firstVertex)
                {
                    owner->min[0] = current[0];
                    owner->min[1] = current[1];
                    owner->min[2] = current[2];
                    owner->max[0] = current[0];
                    owner->max[1] = current[1];
                    owner->max[2] = current[2];
                    owner->firstVertex = false;
                }

                for(int j = 0; j < 3; j++)
                {
                    if(owner->min[j] > current[j]) owner->min[j] = current[j];
                    if(owner->max[j] < current[j]) owner->max[j] = current[j];
                }

                // On en profite pour calculer le rayon
                float dis = sqrtf(current[0] * current[0] + current[1] * current[1] + current[2] * current[2]);

                // Si il est plus grand que le rayon qu'on a déjà on le update
                if(dis > owner->radius) owner->radius = dis;
            }
            break;
        }
        case CHUNK_DKO_NORMAL_ARRAY:
        {
            // On load pour tout les frames
            if(matGroup->interpolatedNA) delete[] matGroup->interpolatedNA;
            matGroup->interpolatedNA = new float[matGroup->nbVertex * 3];
            for(int f = 0; f < parentModel->timeInfo[2]; f++)
            {
                if(matGroup->meshAtFrame[f].normalArray) delete[] matGroup->meshAtFrame[f].normalArray;
                matGroup->meshAtFrame[f].normalArray = new float[matGroup->nbVertex * 3];
                fread(matGroup->meshAtFrame[f].normalArray, matGroup->nbVertex * 3, sizeof(float), ficIn);
            }
            break;
        }
        case CHUNK_DKO_TEXCOORD_ARRAY:
        {
            matGroup->animatedUV = false;
            //  matGroup->texCoordArray = new float [matGroup->nbVertex*2];
            //  fread(matGroup->texCoordArray, matGroup->nbVertex*2, sizeof(float), ficIn);
            if(matGroup->meshAtFrame[0].texCoordArray) delete[] matGroup->meshAtFrame[0].texCoordArray;
            matGroup->meshAtFrame[0].texCoordArray = new float[matGroup->nbVertex * 2];
            fread(matGroup->meshAtFrame[0].texCoordArray, matGroup->nbVertex * 2, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_TEXCOORD_ARRAY_ANIM:
        {
            // On load pour tout les frames
            matGroup->animatedUV = true;
            if(matGroup->interpolatedUV) delete[] matGroup->interpolatedUV;
            matGroup->interpolatedUV = new float[matGroup->nbVertex * 2];
            for(int f = 0; f < parentModel->timeInfo[2]; f++)
            {
                if(matGroup->meshAtFrame[f].texCoordArray) delete[] matGroup->meshAtFrame[f].texCoordArray;
                matGroup->meshAtFrame[f].texCoordArray = new float[matGroup->nbVertex * 2];
                fread(matGroup->meshAtFrame[f].texCoordArray, matGroup->nbVertex * 2, sizeof(float), ficIn);
            }
            break;
        }
        }

        chunkID = readChunk(ficIn);
    }

    // Tout est beau
    return 1;
}



//
// On construit la liste des faces
//
int CdkoMesh::_buildFaceListIt(CFace *faceArray, int index)
{
    int nbFace = 0;

    for(int i = 0; i < nbMatGroup; i++)
    {
        // On cré ses face
        for(int j = 0; j < matGroupArray[i].nbVertex / 3; j++)
        {
            CFace *face = &(faceArray[index + nbFace + j]);

            for(int k = 0; k < 3; k++)
            {
                float tmp[3] = {
                    matGroupArray[i].meshAtFrame[0].vertexArray[j * 9 + k * 3 + 0],
                    matGroupArray[i].meshAtFrame[0].vertexArray[j * 9 + k * 3 + 1],
                    matGroupArray[i].meshAtFrame[0].vertexArray[j * 9 + k * 3 + 2] };

                // On le passe dans la matrice
                CVector right(matrix[0], matrix[1], matrix[2]);
                CVector front(matrix[3], matrix[4], matrix[5]);
                CVector up(matrix[6], matrix[7], matrix[8]);
                CVector current = right * tmp[0] + front * tmp[1] + up * tmp[2] + CVector(position[0], position[1], position[2]);

                face->point[k] = current;
            }

            // On calcul d'abords la normal
            CVector normal = cross(face->point[1] - face->point[0], face->point[2] - face->point[0]);
            normalize(normal);

            // La normal
            face->a = normal[0];
            face->b = normal[1];
            face->c = normal[2];

            // Produit scalaire
            face->d = dot(normal, face->point[0]);

            // For fast collisions
            face->createNormals();

            // Voilà!
        }

        nbFace += matGroupArray[i].nbVertex / 3;
    }

    return nbFace;
}



//
// Pour trouver les vertex à un point donné
//
int CdkoMesh::_buildVertexArrayIt(float * vertexArray, int index)
{
    int nbFloat = 0;
    for(int i = 0; i < nbMatGroup; i++)
    {
        // Ha ah!! on interpolate
        if(parentModel->framef > -1)
        {
            int frameFrom = (int)parentModel->framef;
            int frameTo = (int)parentModel->framef + 1;
            if(frameTo >= parentModel->timeInfo[2]) frameTo = 0;
            float percent = parentModel->framef - (float)(int)parentModel->framef;

            for(int n = 0; n < matGroupArray[i].nbVertex * 3; n++)
            {
                matGroupArray[i].interpolatedVA[n] =
                    matGroupArray[i].meshAtFrame[frameFrom].vertexArray[n] +
                    (matGroupArray[i].meshAtFrame[frameTo].vertexArray[n] -
                        matGroupArray[i].meshAtFrame[frameFrom].vertexArray[n]) * percent;
            }

            matGroupArray[i].ptrVA = matGroupArray[i].interpolatedVA;
        }
        else
        {
            matGroupArray[i].ptrVA = matGroupArray[i].meshAtFrame[parentModel->currentFrame].vertexArray;
        }

        // On cré ses face
        for(int j = 0; j < matGroupArray[i].nbVertex; j++)
        {
            float * ptrArray = &(vertexArray[index + nbFloat + j * 3]);

            float tmp[3] = {
                matGroupArray[i].ptrVA[j * 3 + 0],
                matGroupArray[i].ptrVA[j * 3 + 1],
                matGroupArray[i].ptrVA[j * 3 + 2] };

            // On le passe dans la matrice
            CVector right(matrix[0], matrix[1], matrix[2]);
            CVector front(matrix[3], matrix[4], matrix[5]);
            CVector up(matrix[6], matrix[7], matrix[8]);
            CVector current = right * tmp[0] + front * tmp[1] + up * tmp[2] + CVector(position[0], position[1], position[2]);

            ptrArray[0] = current[0];
            ptrArray[1] = current[1];
            ptrArray[2] = current[2];
        }

        nbFloat += matGroupArray[i].nbVertex * 3;
    }

    return nbFloat;
}



//
// Pour le dessiner
//
void CdkoMesh::drawIt()
{

    // On passe chaque material Group
    for(int i = 0; i < nbMatGroup; i++)
    {
        // On interpolate si c'est nécéssaire
        if(parentModel->framef > -1)
        {
            // Ha ah!! on interpolate
            int frameFrom = (int)parentModel->framef;
            int frameTo = (int)parentModel->framef + 1;
            if(frameTo >= parentModel->timeInfo[2]) frameTo = 0;
            float percent = parentModel->framef - (float)(int)parentModel->framef;
            int nbFloat = matGroupArray[i].nbVertex * 3;

            for(int n = 0; n < nbFloat; n++)
            {
                matGroupArray[i].interpolatedVA[n] =
                    matGroupArray[i].meshAtFrame[frameFrom].vertexArray[n] +
                    (matGroupArray[i].meshAtFrame[frameTo].vertexArray[n] -
                        matGroupArray[i].meshAtFrame[frameFrom].vertexArray[n]) * percent;
                matGroupArray[i].interpolatedNA[n] =
                    matGroupArray[i].meshAtFrame[frameFrom].normalArray[n] +
                    (matGroupArray[i].meshAtFrame[frameTo].normalArray[n] -
                        matGroupArray[i].meshAtFrame[frameFrom].normalArray[n]) * percent;
            }
            if(matGroupArray[i].animatedUV)
            {
                nbFloat = matGroupArray[i].nbVertex * 2;
                for(int n = 0; n < nbFloat; n++)
                {
                    matGroupArray[i].interpolatedUV[n] =
                        matGroupArray[i].meshAtFrame[frameFrom].texCoordArray[n] +
                        (matGroupArray[i].meshAtFrame[frameTo].texCoordArray[n] -
                            matGroupArray[i].meshAtFrame[frameFrom].texCoordArray[n]) * percent;
                }
                matGroupArray[i].ptrUV = matGroupArray[i].interpolatedUV;
            }
            else
            {
                matGroupArray[i].ptrUV = matGroupArray[i].meshAtFrame[0].texCoordArray;
            }

            matGroupArray[i].ptrVA = matGroupArray[i].interpolatedVA;
            matGroupArray[i].ptrNA = matGroupArray[i].interpolatedNA;
        }
        else
        {
            matGroupArray[i].ptrVA = matGroupArray[i].meshAtFrame[parentModel->currentFrame].vertexArray;
            matGroupArray[i].ptrNA = matGroupArray[i].meshAtFrame[parentModel->currentFrame].normalArray;
            if(matGroupArray[i].animatedUV) matGroupArray[i].ptrUV = matGroupArray[i].meshAtFrame[parentModel->currentFrame].texCoordArray;
            else matGroupArray[i].ptrUV = matGroupArray[i].meshAtFrame[0].texCoordArray;
        }

        if(CDko::renderStateBitField & DKO_BUMP_MAP && matGroupArray[i].material &&
            CDko::renderStateBitField & DKO_MULTIPASS && CDko::renderStateBitField & DKO_DYNAMIC_LIGHTING)
        {
            if(matGroupArray[i].material->textureMat)
            {
                if(matGroupArray[i].material->textureMat->bumpMap.textureID)
                {
                    drawBumpFull(i);
                    continue;
                }
            }
        }

#ifndef DEDICATED_SERVER
        glPushAttrib(GL_ENABLE_BIT);
        if(!(CDko::renderStateBitField & DKO_DYNAMIC_LIGHTING)) glDisable(GL_LIGHTING);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, matGroupArray[i].ptrVA);
        if(matGroupArray[i].ptrNA)
        {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, 0, matGroupArray[i].ptrNA);
        }
        if(matGroupArray[i].ptrUV)
        {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 0, matGroupArray[i].ptrUV);
        }

        if(matGroupArray[i].material)
        {
            matGroupArray[i].material->setDiffusePass();
            glDrawArrays(GL_TRIANGLES, 0, matGroupArray[i].nbVertex);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, matGroupArray[i].nbVertex);
        }
        glPopAttrib();

        glDepthMask(GL_FALSE);

        // On mets le detail map
        if(matGroupArray[i].material && CDko::renderStateBitField & DKO_MULTIPASS)
        {
            if(CDko::renderStateBitField & DKO_DETAIL_MAP)
            {
                if(matGroupArray[i].material->textureMat)
                {
                    if(matGroupArray[i].material->textureMat->detailMap.textureID)
                    {
                        matGroupArray[i].material->setDetailPass();
                        glDrawArrays(GL_TRIANGLES, 0, matGroupArray[i].nbVertex);
                        glPopMatrix();
                        glMatrixMode(GL_MODELVIEW);
                        glPopAttrib();
                    }
                }
            }

            if(CDko::renderStateBitField & DKO_SPECULAR &&
                CDko::renderStateBitField & DKO_DYNAMIC_LIGHTING)
            {
                matGroupArray[i].material->setSpecularPass();
                glDrawArrays(GL_TRIANGLES, 0, matGroupArray[i].nbVertex);
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                glPopAttrib();
            }

            if(CDko::renderStateBitField & DKO_SELFILL_MAP)
            {
                if(matGroupArray[i].material->textureMat)
                {
                    if(matGroupArray[i].material->textureMat->selfIlluminationMap.textureID)
                    {
                        matGroupArray[i].material->setSelfIllPass();
                        glDrawArrays(GL_TRIANGLES, 0, matGroupArray[i].nbVertex);
                        glPopMatrix();
                        glMatrixMode(GL_MODELVIEW);
                        glPopAttrib();
                    }
                }
                else if(matGroupArray[i].material->texSelfIll)
                {
                    matGroupArray[i].material->setSelfIllPass();
                    glDrawArrays(GL_TRIANGLES, 0, matGroupArray[i].nbVertex);
                    glPopMatrix();
                    glMatrixMode(GL_MODELVIEW);
                    glPopAttrib();
                }
            }
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDepthMask(GL_TRUE);
        glPopAttrib();
#endif
    }
}



//
// Pour dessiner multi-pass avec le bump pis toute
//
void CdkoMesh::drawBumpFull(int i)
{
    (void)i;
    // On fait lumière par lumière
/*  bool enabledLight[8];
    for (int j=0;j<8;j++)
    {
        enabledLight[j] = glIsEnabled(GL_LIGHT0+j);
        glDisable(GL_LIGHT0+j);
    }

    // On débute le array tout suite
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, matGroupArray[i].vertexArray);

    // Maintenant on enable les lights une par une et on fait les pass nécéssaire
    for (int l=0;l<8;l++)
    {
        if (enabledLight[l])
        {
            // Le tout premier pass doit être celui du bump (le plus toff quoi)
            // Ici on va avoir besoin du vertex shader hey oui...
    //      glDisable(GL_
        }
    }*/
}



//
// Constructeur / Destructeur
//
CdkoMaterial::CdkoMaterial()
{
    textureMat = 0;
    matName = 0;
    shininess = 0;
    transparency = 0;
    twoSided = false;
    wire = false;
    wireSize = 1;
    texDiffuse = 0;
    texBump = 0;
    texSpecular = 0;
    texSelfIll = 0;

    for(int i = 0; i < 3; i++)
    {
        ambient[i] = 0;
        diffuse[i] = 1;
        specular[i] = 0;
        emissive[i] = 0;
    }
    ambient[3] = 1;
    diffuse[3] = 1;
    specular[3] = 1;
    emissive[3] = 1;
}

CdkoMaterial::~CdkoMaterial()
{
    if(textureMat) delete textureMat;
    if(matName) delete matName;

#ifndef DEDICATED_SERVER
    dktDeleteTexture(&texDiffuse);
    dktDeleteTexture(&texBump);
    dktDeleteTexture(&texSpecular);
    dktDeleteTexture(&texSelfIll);
#endif
}



//
// Les pass pour les différentes couches du rendu
//
void CdkoMaterial::setDiffusePass()
{
#ifndef DEDICATED_SERVER
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient); // L'ambient
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse); // La couleur diffuse
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular); // La couleur et intensité spécular
    glMaterialfv(GL_FRONT, GL_EMISSION, emissive); // Le seft illumination
    glMateriali(GL_FRONT, GL_SHININESS, shininess); // L'intensité du spécular : reflet de lumière

    glPushAttrib(GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    if((textureMat || texDiffuse) && CDko::renderStateBitField & DKO_TEXTURE_MAP)
    {
        glEnable(GL_TEXTURE_2D);
        if(textureMat)
        {
            glBindTexture(GL_TEXTURE_2D, textureMat->baseMap.textureID);
            glScalef(1 / textureMat->baseMap.scale, 1 / textureMat->baseMap.scale, 1);
        }
        else if(texDiffuse) glBindTexture(GL_TEXTURE_2D, texDiffuse);
        glColor3f(1, 1, 1); // La texture remplace la diffuse
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor4fv(diffuse);
    }


    // Si on force le clamping
    if(CDko::renderStateBitField & DKO_CLAMP_TEXTURE)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }


    if(twoSided) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
    if(wire || CDko::renderStateBitField & DKO_FORCE_WIREFRAME)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(wireSize);
    }
#endif
}

void CdkoMaterial::setDetailPass()
{
#ifndef DEDICATED_SERVER
    glPushAttrib(GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, textureMat->detailMap.textureID);
    glScalef(1 / textureMat->detailMap.scale, 1 / textureMat->detailMap.scale, 1);
    glColor3f(1, 1, 1); // La texture remplace la diffuse
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);

    if(twoSided) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
    if(wire || CDko::renderStateBitField & DKO_FORCE_WIREFRAME)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(wireSize);
    }
#endif
}

void CdkoMaterial::setSpecularPass()
{
#ifndef DEDICATED_SERVER
    float zero[] = { 0,0,0,1 };
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero); // L'ambient
    glMaterialfv(GL_FRONT, GL_DIFFUSE, zero); // La couleur diffuse
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular); // La couleur et intensité spécular
    glMaterialfv(GL_FRONT, GL_EMISSION, zero); // Le seft illumination
    glMateriali(GL_FRONT, GL_SHININESS, shininess); // L'intensité du spécular : reflet de lumière

    glPushAttrib(GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    if((textureMat || texSpecular) && CDko::renderStateBitField & DKO_SPECULAR_MAP)
    {
        //  float one[] = {1,1,1,1};
        //  glMaterialfv(GL_FRONT,GL_SPECULAR,one); // La texture remplace la couleur spéculaire
        glEnable(GL_TEXTURE_2D);
        if(textureMat)
        {
            glBindTexture(GL_TEXTURE_2D, textureMat->specularMap.textureID);
            glScalef(1 / textureMat->specularMap.scale, 1 / textureMat->specularMap.scale, 1);
        }
        else if(texSpecular) glBindTexture(GL_TEXTURE_2D, texSpecular);
        glColor3f(1, 1, 1); // La texture remplace la diffuse
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor4fv(specular); // La couleur du specular
    }
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_ALPHA, GL_ONE);

    if(twoSided) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
    if(wire || CDko::renderStateBitField & DKO_FORCE_WIREFRAME)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(wireSize);
    }
#endif
}

void CdkoMaterial::setSelfIllPass()
{
#ifndef DEDICATED_SERVER
    glPushAttrib(GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    if(textureMat)
    {
        glBindTexture(GL_TEXTURE_2D, textureMat->selfIlluminationMap.textureID);
        glScalef(1 / textureMat->selfIlluminationMap.scale, 1 / textureMat->selfIlluminationMap.scale, 1);
    }
    else if(texSelfIll) glBindTexture(GL_TEXTURE_2D, texSelfIll);
    glColor3f(1, 1, 1); // La texture remplace la diffuse
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    if(twoSided) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
    if(wire || CDko::renderStateBitField & DKO_FORCE_WIREFRAME)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(wireSize);
    }
#endif
}



//
// Pour loader le materiel
//
int CdkoMaterial::loadFromFile(FILE *ficIn, char *path)
{
    // On load chunk par chunk jusqu'à ce qu'on pogne le chunk End
    short chunkID = readChunk(ficIn);

    while(chunkID != CHUNK_DKO_END)
    {
        switch(chunkID)
        {
        case CHUNK_DKO_MATNAME:
        {
            matName = readString(ficIn);
            break;
        }
        case CHUNK_DKO_TEX_DKT:
        {
            char *dktFilename = readString(ficIn);
            char strTMP[512];
            sprintf(strTMP, "%s%s", path, dktFilename);
            textureMat = new ePTexture();
            textureMat->loadIt(strTMP);
            delete[] dktFilename;
            break;
        }
        case CHUNK_DKO_TEX_DIFFUSE:
        {
            char *texFilename = readString(ficIn);
            char strTMP[512];
            sprintf(strTMP, "%s%s", path, texFilename);
#ifndef DEDICATED_SERVER
            texDiffuse = dktCreateTextureFromFile(strTMP, DKT_FILTER_BILINEAR);
#endif
            delete[] texFilename;
            break;
        }
        case CHUNK_DKO_TEX_BUMP:
        {
            char *texFilename = readString(ficIn);
            char strTMP[512];
            sprintf(strTMP, "%s%s", path, texFilename);
#ifndef DEDICATED_SERVER
            texBump = dktCreateTextureFromFile(strTMP, DKT_FILTER_BILINEAR);
#endif
            delete[] texFilename;
            break;
        }
        case CHUNK_DKO_TEX_SPECULAR:
        {
            char *texFilename = readString(ficIn);
            char strTMP[512];
            sprintf(strTMP, "%s%s", path, texFilename);
#ifndef DEDICATED_SERVER
            texSpecular = dktCreateTextureFromFile(strTMP, DKT_FILTER_BILINEAR);
#endif
            delete[] texFilename;
            break;
        }
        case CHUNK_DKO_TEX_SELFILL:
        {
            char *texFilename = readString(ficIn);
            char strTMP[512];
            sprintf(strTMP, "%s%s", path, texFilename);
#ifndef DEDICATED_SERVER
            texSelfIll = dktCreateTextureFromFile(strTMP, DKT_FILTER_BILINEAR);
#endif
            delete[] texFilename;
            break;
        }
        case CHUNK_DKO_AMBIENT:
        {
            fread(ambient, 4, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_DIFFUSE:
        {
            fread(diffuse, 4, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_SPECULAR:
        {
            fread(specular, 4, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_EMISSIVE:
        {
            fread(emissive, 4, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_SHININESS:
        {
            fread(&shininess, 1, sizeof(short), ficIn);
            break;
        }
        case CHUNK_DKO_TRANSPARENCY:
        {
            fread(&transparency, 1, sizeof(float), ficIn);
            break;
        }
        case CHUNK_DKO_TWO_SIDED:
        {
            char temp;
            fread(&temp, 1, sizeof(char), ficIn);
            twoSided = (temp) ? true : false;
            break;
        }
        case CHUNK_DKO_WIRE_FRAME:
        {
            char temp;
            fread(&temp, 1, sizeof(char), ficIn);
            wire = (temp) ? true : false;
            break;
        }
        case CHUNK_DKO_WIRE_WIDTH:
        {
            fread(&wireSize, 1, sizeof(float), ficIn);
            break;
        }
        }

        chunkID = readChunk(ficIn);
    }

    // Tout est beau
    return 1;
}



//
// Constructeur / Destructeur
//
eHierarchic::eHierarchic()
{
    next = 0;
    childList = 0;
    parent = 0;
    enable = true;
    position[0] = 0;
    position[1] = 0;
    position[2] = 0;
    matrix[0] = 1;
    matrix[1] = 0;
    matrix[2] = 0;
    matrix[3] = 0;
    matrix[4] = 1;
    matrix[5] = 0;
    matrix[6] = 0;
    matrix[7] = 0;
    matrix[8] = 1;
}
eHierarchic::~eHierarchic()
{
    // On efface aussi tout ses enfants
    eHierarchic* ToKill;
    for(eHierarchic* ptrObject = childList; ptrObject; delete ToKill)
    {
        ToKill = ptrObject;
        ptrObject = ptrObject->next;
    }
}



//
// Pour ajouter un enfant
//
void eHierarchic::addChild(eHierarchic* child)
{
    // On le d�ache d'abords de son parent
    if(child->parent) child->detach();

    // Maintenant on l'ajoute �la fin de la liste
    if(!childList)
    {
        child->next = childList;
        childList = child;
    }
    else
    {
        for(eHierarchic* ptrObject = childList; ptrObject; ptrObject = ptrObject->next)
        {
            if(!ptrObject->next)
            {
                ptrObject->next = child;
                break;
            }
        }
    }

    child->parent = this;
}



//
// Pour setter si il est enable ou non
//
void eHierarchic::setEnable(bool value)
{
    // On le set lui en premier
    enable = value;

    // Ensuite ses childs
    for(eHierarchic* ptrObject = childList; ptrObject; ptrObject = ptrObject->next)
        ptrObject->setEnable(value);
}



//
// Pour le d�acher du parent
//
void eHierarchic::detach()
{
    if(parent)
    {
        if(this == parent->childList)
        {
            parent->childList = next;
        }
        else
        {
            for(eHierarchic* ptrObject = parent->childList; ptrObject; ptrObject = ptrObject->next)
            {
                if(ptrObject->next == this)// BUG: it was '=' instead of '=='.
                {
                    ptrObject->next = next;
                }
            }
        }
        parent = 0;
        next = 0;
    }
}



//
// Pour retirer une enfant de sa liste
//
void eHierarchic::removeChild(eHierarchic* child)
{
    if(child)
    {
        child->detach();
    }
}



//
// Pour les dessiner tous
//
void eHierarchic::drawAll()
{
    if(enable)
    {
        // On fait son push pop
#ifndef DEDICATED_SERVER
        glPushMatrix();
        glTranslatef(position[0], position[1], position[2]);
        float Matrix[16] = {
            matrix[0], matrix[1], matrix[2], 0,
            matrix[3], matrix[4], matrix[5], 0,
            matrix[6], matrix[7], matrix[8], 0,
            0,          0,          0,          1 };
        glMultMatrixf(Matrix);
#endif

        // On le dessine lui en premier
        drawIt();

        // Ensuite ses childs
        for(eHierarchic* ptrObject = childList; ptrObject; ptrObject = ptrObject->next)
            ptrObject->drawAll(); // hum pas vraiment d'hierarchi �l'interieur d'un dko mais bon
#ifndef DEDICATED_SERVER
        glPopMatrix();
#endif
    }
}



//
// Pour les effectuer tous
//
void eHierarchic::doAll()
{
    if(enable)
    {
        // Les childs en premier
        for(eHierarchic* ptrObject = childList; ptrObject; ptrObject = ptrObject->next)
            ptrObject->doAll();

        // Le parent en dernier
        doIt();
    }
}



//
// On trouve toute les faces
//
int eHierarchic::_buildFaceList(CFace *faceArray, int index)
{
    int nbAdded = _buildFaceListIt(faceArray, index);

    if(enable)
    {
        // Les childs en premier
        for(eHierarchic* ptrObject = childList; ptrObject; ptrObject = ptrObject->next)
        {
            nbAdded += ptrObject->_buildFaceList(faceArray, index + nbAdded);
        }
    }

    return nbAdded;
}



//
// On rempli son array de vertex array
//
int eHierarchic::_buildVertexArray(float *vertexArray, int index)
{
    int nbAdded = _buildVertexArrayIt(vertexArray, index);

    if(enable)
    {
        // Les childs en premier
        for(eHierarchic* ptrObject = childList; ptrObject; ptrObject = ptrObject->next)
        {
            nbAdded += ptrObject->_buildVertexArray(vertexArray, index + nbAdded);
        }
    }

    return nbAdded;
}


//
// Constructeur
//
ePTexture::ePTexture()
{
    name = 0;
}



//
// Pour enregistrer en .dkt
//

// La struct pour les chunks
struct _typChunkDKT
{
    unsigned short chunkID;
    int32_t lenght;
    _typChunkDKT()
    {
        chunkID = 0;
        lenght = 0;
    }
};

// Une strucs pour saver plus rapidement les données
struct _typDimension
{
    short width;
    short height;
    short bpp;
    _typDimension()
    {
        width = 0;
        height = 0;
        bpp = 0;
    }
};

// Une strucs pour saver les transformations appliqué sur la texture
struct _typTransformation
{
    float scaleU;
    float scaleV;
    float posU;
    float posV;
    _typTransformation()
    {
        scaleU = 0;
        scaleV = 0;
        posU = 0;
        posV = 0;
    }
};

// La définition des chunks
const unsigned short CHUNK_VERSION = 0x0000;
const unsigned short CHUNK_MAP = 0x1000;
const unsigned short CHUNK_MAP_NAME = 0x1100;
const unsigned short CHUNK_MAP_TEXTURE = 0x1200;
const unsigned short CHUNK_MAP_TEXTURE_DIMENSION = 0x1210;
const unsigned short CHUNK_MAP_TEXTURE_TRANSFORMATION = 0x1220;
const unsigned short CHUNK_MAP_TEXTURE_DATA = 0x1230;
const unsigned short CHUNK_MAP_TEXTURE_END = 0x1290;
const unsigned short CHUNK_MAP_END = 0x1900;
const unsigned short CHUNK_VERSION_END = 0x9000;




//
// Pour le loader
//
void ePTexture::loadIt(char* filename)
{
    // On ouvre le fichier
    FILE* ficIn = fopen(filename, "rb");

    // Si l'ouverture a échoué
    if(!ficIn) return;

    // On check la version (ceci doit être le premier chunk, sinon ce n'est pas un fichier valide
    _typChunkDKT chunk;
    fread(&chunk, 1, sizeof(chunk), ficIn);

    if(chunk.chunkID == CHUNK_VERSION)
    {
        // La première version
        short version;
        fread(&version, 1, 2, ficIn);
        if(version != 0x0001) return;
    }
    else
    {
        return;
    }

    // On li le chunk
    fread(&chunk, 1, sizeof(chunk), ficIn);

    // Ensuite on passe tout les chunk jusqu'à fin
    while(chunk.chunkID != CHUNK_VERSION_END)
    {
        // On load une map
        if(chunk.chunkID == CHUNK_MAP)
        {
            loadMap(ficIn);
        }

        // On li le chunk
        fread(&chunk, 1, sizeof(chunk), ficIn);
    }

    // On ferme le fichier
    fclose(ficIn);
}

void ePTexture::loadMap(FILE* ficIn)
{
    _typLayer* ptrLayer = 0;

    // On li le premier chunk
    _typChunkDKT chunk;
    fread(&chunk, 1, sizeof(chunk), ficIn);

    while(chunk.chunkID != CHUNK_MAP_END)
    {
        switch(chunk.chunkID)
        {
        case CHUNK_MAP_NAME:
        {
            char tmp[256];
            loadString(tmp, ficIn);
            if(stricmp(tmp, "basemap") == 0) ptrLayer = &baseMap;
            if(stricmp(tmp, "detailmap") == 0) ptrLayer = &detailMap;
            if(stricmp(tmp, "bumpmap") == 0) ptrLayer = &bumpMap;
            if(stricmp(tmp, "specmap") == 0) ptrLayer = &specularMap;
            if(stricmp(tmp, "selfillmap") == 0) ptrLayer = &selfIlluminationMap;
            break;
        }
        case CHUNK_MAP_TEXTURE:
        {
            loadTexture(ptrLayer, ficIn);
            break;
        }
        }

        // On li le chunk suivant
        fread(&chunk, 1, sizeof(chunk), ficIn);
    }
}

void ePTexture::loadTexture(_typLayer* ptrLayer, FILE* ficIn)
{
    // On li le premier chunk
    _typChunkDKT chunk;
    fread(&chunk, 1, sizeof(chunk), ficIn);

    // On passe les autres apres
    while(chunk.chunkID != CHUNK_MAP_TEXTURE_END)
    {
        switch(chunk.chunkID)
        {
        case CHUNK_MAP_TEXTURE_DIMENSION:
        {
            _typDimension dim;
            fread(&dim, 1, sizeof(_typDimension), ficIn);
            ptrLayer->w = dim.width;
            ptrLayer->h = dim.height;
            ptrLayer->bpp = dim.bpp;
            break;
        }
        case CHUNK_MAP_TEXTURE_TRANSFORMATION:
        {
            _typTransformation trans;
            fread(&trans, 1, sizeof(_typTransformation), ficIn);
            ptrLayer->scale = trans.scaleU;
            break;
        }
        case CHUNK_MAP_TEXTURE_DATA:
        {
#ifndef DEDICATED_SERVER
            GLubyte* imageData = new GLubyte[ptrLayer->w*ptrLayer->h*ptrLayer->bpp];
#else
            unsigned char* imageData = new unsigned char[ptrLayer->w*ptrLayer->h*ptrLayer->bpp];
#endif
            fread(imageData, 1, ptrLayer->w*ptrLayer->h*ptrLayer->bpp, ficIn);
            if(ptrLayer->bpp == 4)
                ptrLayer->textureID = createTextureFromBuffer(imageData, ptrLayer->w, ptrLayer->h, ptrLayer->bpp, 1, false);
            else
                ptrLayer->textureID = createTextureFromBuffer(imageData, ptrLayer->w, ptrLayer->h, ptrLayer->bpp, 2, false);
            break;
        }
        }

        // On li le chunk suivant
        fread(&chunk, 1, sizeof(chunk), ficIn);
    }
}



//
// Loader une string
//
void ePTexture::loadString(char* string, FILE* ficIn)
{
    // On load le premier caractère
    int i = 0;
    fread(&(string[i++]), 1, 1, ficIn);

    // On load le reste
    for(; string[i - 1]; fread(&(string[i++]), 1, 1, ficIn));
}





//
// Créer une texture ogl à partir du buffer
//
//
// On cré une texture à partir d'un buffer
//
unsigned int createTextureFromBuffer(unsigned char *Buffer, int Width, int Height, int BytePerPixel, int Filter, bool inverse)
{
#ifndef DEDICATED_SERVER
    // On cré la texture
    unsigned int Texture = 0;
    GLint Level = (BytePerPixel == 3) ? GL_RGB : GL_RGBA;

    // On génère une texture
    glGenTextures(1, &Texture);

    // On pogne la dernière erreur
    if(Texture == 0)
    {
    }

    // On bind cette texture au context
    glBindTexture(GL_TEXTURE_2D, Texture);

    // On met ça en bilinear (pixel et mipmap)
    switch(Filter)
    {
    case 0: // Nearest
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }
    case 1: // Linear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    }
    case 2: // Bilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    }
    case 3: // Trilinear
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
    default: // Nearest default
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    }
    }

    if(inverse)
    {
        unsigned char temp;

        // Ici c'est con, mais faut switcher le rouge avec le bleu
        for(int32_t i = 0; i < (int32_t)(Width*Height*BytePerPixel); i += BytePerPixel)
        {
            temp = Buffer[i];
            Buffer[i] = Buffer[i + 2];
            Buffer[i + 2] = temp;
        }
    }

    // On construit les mipmap maintenant
    glTexImage2D(GL_TEXTURE_2D, 0, Level, Width, Height, 0, Level, GL_UNSIGNED_BYTE, Buffer);
    glGenerateMipmap(GL_TEXTURE_2D);

    return Texture;
#endif
}
