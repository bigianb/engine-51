#include "gltfExporter.h"
#include "../../a51lib/RigidGeom.h"
#include "../../a51lib/SkinGeom.h"
#include "../../a51lib/Bitmap.h"
#include "../../a51lib/DFSFile.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../../a51lib/gltf/tiny_gltf.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QBuffer>
#include <algorithm>

// Helper function to extract texture data and embed as PNG in glTF image
bool extractAndEmbedTexture(DFSFile* dfsFile, const std::string& textureName, tinygltf::Image& image)
{
    // Extract base name and extension from textureName
    std::string textureBaseName = textureName;
    std::string textureExtension;

    size_t dotPos = textureName.find_last_of('.');
    if (dotPos != std::string::npos) {
        textureBaseName = textureName.substr(0, dotPos);
        textureExtension = textureName.substr(dotPos);
    }

    // Convert to uppercase for comparison
    std::transform(textureBaseName.begin(), textureBaseName.end(), textureBaseName.begin(), ::toupper);
    std::transform(textureExtension.begin(), textureExtension.end(), textureExtension.begin(), ::toupper);

    // Find the texture file in the DFS
    int textureIndex = -1;
    for (int i = 0; i < dfsFile->numFiles(); ++i) {
        std::string baseName = dfsFile->getBaseFilename(i);
        std::string extension = dfsFile->getFileExtension(i);

        // Convert to uppercase for comparison
        std::transform(baseName.begin(), baseName.end(), baseName.begin(), ::toupper);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::toupper);

        if (baseName == textureBaseName && extension == textureExtension) {
            textureIndex = i;
            break;
        }
    }

    if (textureIndex == -1) {
        return false; // Texture not found
    }

    // Load the texture data
    uint8_t* fileData = dfsFile->getFileData(textureIndex);
    int fileLen = dfsFile->getFileSize(textureIndex);

    if (!fileData || fileLen == 0) {
        return false;
    }

    // Load into Bitmap
    Bitmap bitmap;
    const bool oldVersion = dfsFile->getVersion() == 1;
    if (!bitmap.readFile(fileData, fileLen, oldVersion)) {
        return false;
    }

    // Let's try staying with the bitmap's original format and using RGB888
    bitmap.convertFormat(Bitmap::FMT_24_RGB_888);

    // Create QImage from RGB data
    QImage qimage(bitmap.data.pixelData, bitmap.getWidth(), bitmap.getHeight(), bitmap.getWidth() * 3, QImage::Format_RGB888);

    // Save to QByteArray as PNG
    QByteArray pngData;
    QBuffer buffer(&pngData);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, "PNG");
    if (!writer.write(qimage)) {
        return false;
    }

    // Embed the PNG data in the glTF image using base64 data URI
    QByteArray base64Data = pngData.toBase64();
    std::string dataUri = "data:image/png;base64," + std::string(base64Data.constData(), base64Data.size());

    // Clear all other properties and set only the data URI
    image = tinygltf::Image(); // Reset to clean state
    image.uri = dataUri;

    return true;
}

void exportGLTF(RigidGeom& rigidGeom, QString fileName, DFSFile* dfsFile)
{
    tinygltf::Model m;
    tinygltf::Scene scene;

    int numMeshes = rigidGeom.getNumMeshes();
    int accessorIdx = 0;
    int viewIdx = 0;
    int materialIdx = 0;
    int textureIdx = 0;

    for (int texNo = 0; texNo < rigidGeom.getNumTextures(); ++texNo) {
        tinygltf::Image image;
        std::string tfn = rigidGeom.getTextureFilename(texNo);

        // Extract and embed texture if DFS file is available
        if (dfsFile && extractAndEmbedTexture(dfsFile, tfn, image)) {
            // Successfully embedded - image object is already populated
        } else {
            // Embedding failed or no DFS file - create clean image with URI
            image = tinygltf::Image(); // Reset to clean state
            std::string pngName = tfn;
            if (pngName.length() >= 4) {
                pngName.replace(pngName.end() - 4, pngName.end(), "png");
            }
            for (auto& c : pngName) {
                c = toupper(c);
            }
            image.uri = pngName;
        }

        m.images.push_back(image);
    }

    int nodeMeshIdx = 0;
    int buffersIdx = 0;
    for (int meshNo = 0; meshNo < numMeshes; ++meshNo) {
        Mesh& mesh = rigidGeom.meshes[meshNo];
        for (int submeshIdx = mesh.iSubMesh; submeshIdx < mesh.iSubMesh + mesh.nSubMeshes; ++submeshIdx) {

            int    numVertices = rigidGeom.getNumSubmeshVertices(submeshIdx);
            float* vertexData = rigidGeom.getSubmeshVerticesPUV(submeshIdx);
            float* normals = rigidGeom.getSubmeshVertexNormals(submeshIdx);

            // This is the raw data buffer.
            // Must be a better way to do this
            const unsigned char* pcvd = (unsigned char*)vertexData;
            tinygltf::Buffer     buffer;
            buffer.data = std::vector<unsigned char>(pcvd, pcvd + numVertices * 20);
            delete[] vertexData;
            m.buffers.push_back(buffer);
            int vBufIdx = buffersIdx++;

            const unsigned char* pcnd = (unsigned char*)normals;
            tinygltf::Buffer     normalsBuffer;
            normalsBuffer.data = std::vector<unsigned char>(pcnd, pcnd + numVertices * 12);
            delete[] normals;
            m.buffers.push_back(normalsBuffer);
            int nBufIndx = buffersIdx++;

            tinygltf::BufferView posBufferView;
            posBufferView.buffer = vBufIdx;
            posBufferView.byteOffset = 0;
            posBufferView.byteLength = numVertices * 12;
            posBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            tinygltf::BufferView uvBufferView;
            uvBufferView.buffer = vBufIdx;
            uvBufferView.byteOffset = numVertices * 12;
            uvBufferView.byteLength = numVertices * 8;
            uvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            m.bufferViews.push_back(posBufferView);
            int posBufferViewIdx = viewIdx++;

            m.bufferViews.push_back(uvBufferView);
            int uvBufferViewIdx = viewIdx++;

            tinygltf::BufferView normalsBufferView;
            normalsBufferView.buffer = nBufIndx;
            normalsBufferView.byteOffset = 0;
            normalsBufferView.byteLength = numVertices * 12;
            normalsBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            m.bufferViews.push_back(normalsBufferView);
            int normalsBufferViewIdx = viewIdx++;

            // Describe the layout of posBufferView, the vertices themself
            tinygltf::Accessor posAccessor;
            posAccessor.bufferView = posBufferViewIdx;
            posAccessor.byteOffset = 0;
            posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            posAccessor.count = numVertices;
            posAccessor.type = TINYGLTF_TYPE_VEC3;
            const BBox& bbox = rigidGeom.getBoundingBox(meshNo);
            posAccessor.maxValues = {bbox.max.x, bbox.max.y, bbox.max.z};
            posAccessor.minValues = {bbox.min.x, bbox.min.y, bbox.min.z};

            tinygltf::Accessor uvAccessor;
            uvAccessor.bufferView = uvBufferViewIdx;
            uvAccessor.byteOffset = 0;
            uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            uvAccessor.count = numVertices;
            uvAccessor.type = TINYGLTF_TYPE_VEC2;
            uvAccessor.maxValues = {1.0, 1.0}; // TODO: Build correct min/max from data
            uvAccessor.minValues = {-1.0, -1.0};

            tinygltf::Accessor normalsAccessor;
            normalsAccessor.bufferView = normalsBufferViewIdx;
            normalsAccessor.byteOffset = 0;
            normalsAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            normalsAccessor.count = numVertices;
            normalsAccessor.type = TINYGLTF_TYPE_VEC3;
            normalsAccessor.maxValues = {1.0, 1.0, 1.0}; // TODO: Build correct min/max from data
            normalsAccessor.minValues = {-1.0, -1.0, -1.0};

            m.accessors.push_back(posAccessor);
            int posAccessorId = accessorIdx++;

            m.accessors.push_back(uvAccessor);
            int uvAccessorId = accessorIdx++;

            m.accessors.push_back(normalsAccessor);
            int normalsAccessorId = accessorIdx++;

            tinygltf::Texture texture;
            int               meshMatIdx = rigidGeom.subMeshes[submeshIdx].iMaterial;
            texture.source = rigidGeom.materials[meshMatIdx].iTexture; // Points to image index
            m.textures.push_back(texture);
            int currentTextureIdx = m.textures.size() - 1;

            // Create a simple material
            tinygltf::Material mat;
            mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 0.9f, 0.9f, 1.0f};
            mat.pbrMetallicRoughness.baseColorTexture.index = currentTextureIdx;

            mat.doubleSided = true;
            m.materials.push_back(mat);
            int theMaterialIdx = materialIdx++;
            textureIdx++;

            // Build the mesh primitive and add it to the mesh
            tinygltf::Primitive primitive;
            primitive.attributes["POSITION"] = posAccessorId;
            primitive.attributes["TEXCOORD_0"] = uvAccessorId;
            primitive.attributes["NORMAL"] = normalsAccessorId;
            primitive.material = theMaterialIdx;
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            tinygltf::Mesh mesh;
            mesh.primitives.push_back(primitive);
            m.meshes.push_back(mesh);

            // Other tie ups
            tinygltf::Node node;
            node.mesh = nodeMeshIdx;
            scene.nodes.push_back(nodeMeshIdx++); // Default scene
            m.nodes.push_back(node);
        }
    }
    m.scenes.push_back(scene);

    // Define the asset. The version is required
    tinygltf::Asset asset;
    asset.version = "2.0";
    asset.generator = "DFSViewer";
    m.asset = asset;

    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&m, fileName.toStdString(),
                              false,  // embedImages (we handle embedding manually)
                              true,   // embedBuffers
                              true,   // pretty print
                              false); // write binary
}

void exportGLTF(SkinGeom& geom, QString fileName, DFSFile* dfsFile)
{
    tinygltf::Model m;
    tinygltf::Scene scene;

    int numMeshes = geom.getNumMeshes();
    int accessorIdx = 0;
    int viewIdx = 0;
    int materialIdx = 0;
    int textureIdx = 0;

    for (int texNo = 0; texNo < geom.getNumTextures(); ++texNo) {
        tinygltf::Image image;
        std::string tfn = geom.getTextureFilename(texNo);

        // Extract and embed texture if DFS file is available
        if (dfsFile && extractAndEmbedTexture(dfsFile, tfn, image)) {
            // Successfully embedded - image object is already populated
        } else {
            // Embedding failed or no DFS file - create clean image with URI
            image = tinygltf::Image(); // Reset to clean state
            std::string pngName = tfn;
            if (pngName.length() >= 4) {
                pngName.replace(pngName.end() - 4, pngName.end(), "png");
            }
            for (auto& c : pngName) {
                c = toupper(c);
            }
            image.uri = pngName;
        }

        m.images.push_back(image);
    }

    int nodeMeshIdx = 0;
    int buffersIdx = 0;
    for (int meshNo = 0; meshNo < numMeshes; ++meshNo) {
        Mesh& mesh = geom.meshes[meshNo];
        for (int submeshIdx = mesh.iSubMesh; submeshIdx < mesh.iSubMesh + mesh.nSubMeshes; ++submeshIdx) {

            int    numVertices = geom.getNumSubmeshVertices(submeshIdx);
            float* vertexData = geom.getSubmeshVerticesPUV(submeshIdx);
            float* normals = geom.getSubmeshVertexNormals(submeshIdx);

            // This is the raw data buffer.
            // Must be a better way to do this
            const unsigned char* pcvd = (unsigned char*)vertexData;
            tinygltf::Buffer     buffer;
            buffer.data = std::vector<unsigned char>(pcvd, pcvd + numVertices * 20);
            delete[] vertexData;
            m.buffers.push_back(buffer);
            int vBufIdx = buffersIdx++;

            const unsigned char* pcnd = (unsigned char*)normals;
            tinygltf::Buffer     normalsBuffer;
            normalsBuffer.data = std::vector<unsigned char>(pcnd, pcnd + numVertices * 12);
            delete[] normals;
            m.buffers.push_back(normalsBuffer);
            int nBufIndx = buffersIdx++;

            tinygltf::BufferView posBufferView;
            posBufferView.buffer = vBufIdx;
            posBufferView.byteOffset = 0;
            posBufferView.byteLength = numVertices * 12;
            posBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            tinygltf::BufferView uvBufferView;
            uvBufferView.buffer = vBufIdx;
            uvBufferView.byteOffset = numVertices * 12;
            uvBufferView.byteLength = numVertices * 8;
            uvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            m.bufferViews.push_back(posBufferView);
            int posBufferViewIdx = viewIdx++;

            m.bufferViews.push_back(uvBufferView);
            int uvBufferViewIdx = viewIdx++;

            tinygltf::BufferView normalsBufferView;
            normalsBufferView.buffer = nBufIndx;
            normalsBufferView.byteOffset = 0;
            normalsBufferView.byteLength = numVertices * 12;
            normalsBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

            m.bufferViews.push_back(normalsBufferView);
            int normalsBufferViewIdx = viewIdx++;

            // Describe the layout of posBufferView, the vertices themself
            tinygltf::Accessor posAccessor;
            posAccessor.bufferView = posBufferViewIdx;
            posAccessor.byteOffset = 0;
            posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            posAccessor.count = numVertices;
            posAccessor.type = TINYGLTF_TYPE_VEC3;
            const BBox& bbox = geom.getBoundingBox(meshNo);
            posAccessor.maxValues = {bbox.max.x, bbox.max.y, bbox.max.z};
            posAccessor.minValues = {bbox.min.x, bbox.min.y, bbox.min.z};

            tinygltf::Accessor uvAccessor;
            uvAccessor.bufferView = uvBufferViewIdx;
            uvAccessor.byteOffset = 0;
            uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            uvAccessor.count = numVertices;
            uvAccessor.type = TINYGLTF_TYPE_VEC2;
            uvAccessor.maxValues = {1.0, 1.0}; // TODO: Build correct min/max from data
            uvAccessor.minValues = {-1.0, -1.0};

            tinygltf::Accessor normalsAccessor;
            normalsAccessor.bufferView = normalsBufferViewIdx;
            normalsAccessor.byteOffset = 0;
            normalsAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            normalsAccessor.count = numVertices;
            normalsAccessor.type = TINYGLTF_TYPE_VEC3;
            normalsAccessor.maxValues = {1.0, 1.0, 1.0}; // TODO: Build correct min/max from data
            normalsAccessor.minValues = {-1.0, -1.0, -1.0};

            m.accessors.push_back(posAccessor);
            int posAccessorId = accessorIdx++;

            m.accessors.push_back(uvAccessor);
            int uvAccessorId = accessorIdx++;

            m.accessors.push_back(normalsAccessor);
            int normalsAccessorId = accessorIdx++;

            tinygltf::Texture texture;
            int               meshMatIdx = geom.subMeshes[submeshIdx].iMaterial;
            texture.source = geom.materials[meshMatIdx].iTexture; // Points to image index
            m.textures.push_back(texture);
            int currentTextureIdx = m.textures.size() - 1;

            // Create a simple material
            tinygltf::Material mat;
            mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 0.9f, 0.9f, 1.0f};
            mat.pbrMetallicRoughness.baseColorTexture.index = currentTextureIdx;

            mat.doubleSided = true;
            m.materials.push_back(mat);
            int theMaterialIdx = materialIdx++;
            textureIdx++;

            // Build the mesh primitive and add it to the mesh
            tinygltf::Primitive primitive;
            primitive.attributes["POSITION"] = posAccessorId;
            primitive.attributes["TEXCOORD_0"] = uvAccessorId;
            primitive.attributes["NORMAL"] = normalsAccessorId;
            primitive.material = theMaterialIdx;
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            tinygltf::Mesh mesh;
            mesh.primitives.push_back(primitive);
            m.meshes.push_back(mesh);

            // Other tie ups
            tinygltf::Node node;
            node.mesh = nodeMeshIdx;
            scene.nodes.push_back(nodeMeshIdx++); // Default scene
            m.nodes.push_back(node);
        }
    }
    m.scenes.push_back(scene);

    // Define the asset. The version is required
    tinygltf::Asset asset;
    asset.version = "2.0";
    asset.generator = "DFSViewer";
    m.asset = asset;

    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&m, fileName.toStdString(),
                              false,  // embedImages (we handle embedding manually)
                              true,   // embedBuffers
                              true,   // pretty print
                              false); // write binary
}
