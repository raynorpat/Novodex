#ifndef SG_TERRAIN_H
#define SG_TERRAIN_H
namespace SceneGraph {
class Mesh;
// Load CHU versions 8/9 as a finest-resolution indexed triangle mesh. CHU has
// no material records; its surface uses the "default" group. OBJ/MESH aliases
// retain their original groups. Failure throws std::runtime_error.
void loadTerrain(Mesh& destination,const char* filename,bool textureCoords=true,
                 float scale=0,bool forceNormals=true,unsigned forceTexCoordType=0);
}
#endif
