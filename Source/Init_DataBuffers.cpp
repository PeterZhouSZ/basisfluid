
#include "Application.h"

#include "VectorField2D.h"

#include <memory>

using namespace glm;
using namespace std;

bool Application::Init_DataBuffers() {

    _velocityField = make_unique<VectorField2D>(
        _domainLeft, _domainRight, _domainBottom, _domainTop,
        _nbCellsVelocity, _nbCellsVelocity);
    _velocityField->createVectorCpuStorage();
    _velocityField->createVectorTexture2DStorage(
        GL_RG, GL_RG32F, GL_RG, GL_FLOAT, 1);
    _velocityField->populateWithFunction([=](float /*x*/, float /*y*/) {
        return vec2(0);
    });

    _forceField = make_unique<VectorField2D>(_domainLeft, _domainRight, _domainBottom, _domainTop,
        _forcesGridRes, _forcesGridRes);
    _forceField->createVectorCpuStorage();
    _forceField->createVectorTexture2DStorage(GL_RG, GL_RG32F, GL_RG, GL_FLOAT, 1);

    //all translated basis flows parameters
    _basisFlowParams = make_unique<DataBuffer1D<BasisFlow>>(1);
    _basisFlowParams->createCpuStorage();
    _basisFlowParams->createBufferStorage(GL_FLOAT, sizeof(BasisFlow) / sizeof(float));
    _basisFlowParams->resize(0);

    //initialize basis flows. all basis templated are centered at (0,0), freq
    //1-1 has support [-0.5,0.5]^2 and other frequencies have smaller supports
    //according to their frequencies. We only need to create one basis template
    //per anisotropy ratio, so here we compute freqLvlX = 0 and freqLvlY = iRatio.    
    _basisFlowTemplates = new std::unique_ptr<VectorField2D>[_maxAnisoLvl + 1];
    for (unsigned int iRatio = 0; iRatio < uint(_maxAnisoLvl + 1); iRatio++) {
        _basisFlowTemplates[iRatio] = make_unique<VectorField2D>(
            -0.5f, 0.5f,
            -0.5f / float(1 << iRatio), 0.5f / float(1 << iRatio),
            _nbCellsBasisTemplates, _nbCellsBasisTemplates);
        _basisFlowTemplates[iRatio]->createVectorCpuStorage();
        _basisFlowTemplates[iRatio]->createVectorTexture2DStorage(
            GL_RG, GL_RG32F, GL_RG, GL_FLOAT, 1);
        _basisFlowTemplates[iRatio]->populateWithFunction(
            [=](float x, float y) {
            return vec2(flowBasisHat(dvec2(x, y), iRatio));
        }
        );
    }

    _partPos = make_unique<DataBuffer1D<vec2>>(1);
    _partPos->createCpuStorage();
    _partPos->createBufferStorage(GL_FLOAT, 2);
    _partPos->resize(0);

    _partVecs = make_unique<DataBuffer1D<vec2>>(1);
    _partVecs->createCpuStorage();
    _partVecs->createBufferStorage(GL_FLOAT, 2);
    _partVecs->resize(0);

    _partAges = make_unique<DataBuffer1D<float>>(1);
    _partAges->createCpuStorage();
    _partAges->createBufferStorage(GL_FLOAT, 1);
    _partAges->resize(0);

    // acceleration grid for particles
    _accelParticles = make_unique<GridData2D<vector<unsigned int>*>>(
        _domainLeft, _domainRight,
        _domainBottom, _domainTop,
        _accelParticlesRes, _accelParticlesRes
        );
    _accelParticles->createCpuStorage();
    for (uint i = 0; i < _accelParticlesRes; i++) {
        for (uint j = 0; j < _accelParticlesRes; j++) {
            _accelParticles->setCpuData(i, j, new vector<unsigned int>);
        }
    }

    _bufferGridPoints = make_unique<DataBuffer1D<vec2>>(
        _velocityField->nbElementsX() * _velocityField->nbElementsY());
    _bufferGridPoints->createCpuStorage();
    _bufferGridPoints->createBufferStorage(GL_FLOAT, 2);

    Metadata1DCpu gridNodes = _velocityField->GenerateGridNodeLocations();
    memcpy(
        _bufferGridPoints->getCpuDataPointer(),
        gridNodes.dataPointer,
        _velocityField->nbElementsX()*_velocityField->nbElementsY() * sizeof(vec2));

    _bufferArrows = make_unique<DataBuffer1D<vec2>>(
        _velocityField->nbElementsX() * _velocityField->nbElementsY());
    _bufferArrows->createCpuStorage();
    _bufferArrows->createBufferStorage(GL_FLOAT, 2);

    _obstacleLines = make_unique<DataBuffer1D<vec2>>(1);
    _obstacleLines->createCpuStorage();
    _obstacleLines->createBufferStorage(GL_FLOAT, 2);
    _obstacleLines->resize(0);

    _vecX = make_unique<DataBuffer1D<double>>(0);
    _vecX->createCpuStorage();

    _vecXForces = make_unique<DataBuffer1D<double>>(0);
    _vecXForces->createCpuStorage();

    _vecXBoundaryForces = make_unique<DataBuffer1D<double>>(0);
    _vecXBoundaryForces->createCpuStorage();

    _vecB = make_unique<DataBuffer1D<double>>(0);
    _vecB->createCpuStorage();

    _accelBasisCentersIds = make_unique<DataBuffer2D<std::vector<unsigned int>*>>(_accelBasisRes, _accelBasisRes);
    _accelBasisCentersIds->createCpuStorage();
    for (uint i = 0; i < _accelBasisRes; i++) {
        for (uint j = 0; j < _accelBasisRes; j++) {
            _accelBasisCentersIds->setCpuData(i, j, new vector<unsigned int>);
        }
    }

    _intersectingBasesIds = make_unique<DataBuffer1D<vector<unsigned int>*>>(0);
    _intersectingBasesIds->createCpuStorage();

    _intersectingBasesIdsTransport = make_unique<DataBuffer1D<std::vector<unsigned int>*>>(0);
    _intersectingBasesIdsTransport->createCpuStorage();

    for (int iRelFreq = 0; iRelFreq < _nbExplicitTransferFreqs; iRelFreq++) {
        _intersectingBasesIdsDeformation[iRelFreq] = make_unique<DataBuffer1D<std::vector<CoeffBBDecompressedIntersectionInfo>*>>(0);
        _intersectingBasesIdsDeformation[iRelFreq]->createCpuStorage();
    }

    return true;
}