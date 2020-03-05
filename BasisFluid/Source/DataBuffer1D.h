// 1D array of data. We can decide if the data lives on the CPU or GPU, what is
// format is (templates?). Can be reinterpreted as 2D, 3D, or any of the
// formats in OpenGL (maybe even more).

// The order of texture data e.g. 3D seems to be W.H.z + W.y + x, but I have to
// verify that.

//store on CPU and send to GPU if needed. Then we can map the GPU data to the
//CPU to edit it. If we edit it and the mapped region is not mapped, we
//generate an error. If data is only stores on the GPU, the mapped region is 0
//for X Y and Z. THe DataBuffer can of course be used as a CPU data storage if
//the data is not sent to the GPU.

//For, either store all on CPU, all on GPU, or both and have a system to
//transfer the info. I can then optimize and specialize this later. FOr
//instance, for now the transferCpuDataToGpu and transferGpuDataToCpu functions
//wil update the whole data, but later it will be possible to only transfer a
//certain range of this information. This is trickier for non-1D textures, e.g.
//since a 2D range has to be mapped to multiple 1D slices on the GPU. Don't
//know yet how to implement it, either with bufferSubData stuff o with
//glMapBuffer stuff.

//we use immutable textures, but we can still resize the CPU texture and create
//a new GPU texture with the new size.

//we can store the data on the GPU in a buffer or in a texture, or both.
//Textures are useful for rendering the data, and buffers are useful for
//writting to the data from the GPU, or using the data as e.g. a vertex buffer.
//1D textures can also be stored as buffer textures. 2D textures can have
//texture storage or 1D_array storage. etc for 3D.


#ifndef DATABUFFER1D_H
#define DATABUFFER1D_H

#include "GL/glew.h"

struct Metadata1DCpu {
    void* dataPointer;
};

struct Metadata1DBuffer {
    GLuint bufferId;
    unsigned int nbElements;
    GLenum dataType; // e.g. GL_FLOAT
    unsigned int nbElementsPerComponent; // e.g. 3 for vec3
};



template<class T>
class DataBuffer1D
{
public:
    Metadata1DBuffer _metadataBuffer;
    Metadata1DCpu _metadataCpu;

    unsigned int _capacity;
    unsigned int _nbElements;
    GLuint _glidBuffer;
    GLuint _glidTexture1D;
    GLuint _glidTextureBuffer;
    bool _hasCpuStorage;
    bool _hasBufferStorage;
    bool _hasTexture1DStorage;
    bool _hasTextureBufferStorage;
    GLenum _texture1DInternalFormat; // internal texel format. (http://docs.gl/gl4/glTexStorage1D)
    GLenum _texture1DSizedInternalFormat;
    GLenum _texture1DExternalFormat; // external texel format, used to communicate with user, e.g. set or get the texture data. (http://docs.gl/gl4/glGetTexImage)
    GLenum _texture1DSizedExternalFormat;
    GLenum _textureBufferSizedInternalFormat; // internal texture buffer format. (http://docs.gl/gl4/glTexBuffer)

public:
    T* _dataCpu;
    GLuint dataBuffer;

public:

    DataBuffer1D(unsigned int size);
    void createCpuStorage();
    void deleteCpuStorage();
    void createBufferStorage(GLenum dataType, unsigned int nbElementsPerComponent, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    void deleteBufferStorage();
    void createTexture1DStorage(GLenum internalFormat, GLenum sizedInternalFormat,
                                GLenum externalFormat, GLenum sizedExternalFormat);
    void deleteTexture1DStorage();
    void createTextureBufferStorage(GLenum sizedFormat);
    void deleteTextureBufferStorage();

    void resize(unsigned int size);
    void appendCpu(T elem);

    T getCpuData(unsigned int i);
    T* getCpuDataPointer();
    void setCpuData(unsigned int i, T data);
    void TransferDataCpuToBuffer();
    
    unsigned int dataCpuSizeInBytes() {
        return _nbElements * sizeof(T);
    }
};

// include definitions because the class is templated.
#include "DataBuffer1D.tpp"

#endif // DATABUFFER1D_H
