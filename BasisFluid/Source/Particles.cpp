
#include "Application.h"
#include "Obstacles.h"

#include "glm/gtc/random.hpp"
#include <algorithm>

using namespace std;

#define min4(a,b,c,d) std::min( std::min(a,b), std::min(c,d) )
#define max4(a,b,c,d) std::max( std::max(a,b), std::max(c,d) )
#define min8(a,b,c,d,e,f,g,h) std::min( std::min( std::min(a,b), std::min(c,d) ), std::min( std::min(e,f), std::min(g,h) ) )
#define max8(a,b,c,d,e,f,g,h) std::max( std::max( std::max(a,b), std::max(c,d) ), std::max( std::max(e,f), std::max(g,h) ) )



// clears the acceleration structure for particles, and then palces each particles in the acceleration grid.
void Application::SetParticlesInAccelGrid()
{
    for (uint i = 0; i < _accelParticlesRes; i++) {
        for (uint j = 0; j < _accelParticlesRes; j++) {
            _accelParticles->getCpuData_noRefresh(i, j)->clear();
        }
    }

    vec2* particlesPointer = _partPos->getCpuDataPointer();
    vec2* partVecsPointer = _partVecs->getCpuDataPointer();
    for (uint iPart = 0; iPart < _partPos->_nbElements; ++iPart) {
        //vec2 pos = particles->getCpuData(iPart);
        vec2 pos = particlesPointer[iPart];
        ivec2 gridId = ivec2(_accelParticles->pointToClosestIndex(pos));
        _accelParticles->getCpuData_noRefresh(gridId.x, gridId.y)->push_back(iPart);
        //partVecs->setCpuData(iPart, vec2(0));
        partVecsPointer[iPart] = vec2(0);
    }
    _partPos->_sourceStorageType = DataBuffer1D<vec2>::StorageType::CPU;
    _partVecs->_sourceStorageType = DataBuffer1D<vec2>::StorageType::CPU;
}



// compute particle direction first without moving them (because the velocity of a basis flow on a particle depends on its position). Then apply movement.
void Application::ComputeParticleAdvection()
{

    //particles->refreshCpuData();
    vec2* particlesPointer = _partPos->getCpuDataPointer();
    //partVecs->refreshCpuData();
    vec2* partVecsPointer = _partVecs->getCpuDataPointer();
    //partAges->refreshCpuData();
    float* partAgesPointer = _partAges->getCpuDataPointer();
    //_basisFlowParams->refreshCpuData();
    BasisFlow* basisFlowParamsPointer = _basisFlowParams->getCpuDataPointer();

    bool useLocalVectorFieldPartAdv = true;
    bool pushParticlesOutOfObstacles = true;


    for (uint iSubstep = 0; iSubstep < uint(_substepsParticles); iSubstep++)
    {

        // reset particle movement to 0.
        for (unsigned int iPart = 0; iPart < _partVecs->_nbElements; ++iPart) {
            //partVecs->setCpuData(iPart, vec2(0));
            partVecsPointer[iPart] = vec2(0);
        }



        // accumulate particle movement from basis velocities. Do not acutally move particles yet.
        for (unsigned int iBasis = 0; iBasis < _basisFlowParams->_nbElements; ++iBasis) {
            //BasisFlow b = basisFlowParams->getCpuData(iBasis);
            BasisFlow& b = basisFlowParamsPointer[iBasis];

            // compute range of the basis in the particle acceleration grid, to know what particles to change.
            ivec2 gridIdsMin, gridIdsMax;
            //                if(!b.valid) {
            //                if( !allBitsSet(b.bitFlags, INTERIOR) ) {
            //                if (true) {
            if ((!AllBitsSet(b.bitFlags, INTERIOR)) && (!AllBitsSet(b.bitFlags, DYNAMIC_BOUNDARY_PROJECTION))) {
                continue;
            }
            else if (!b.stretched) {
                gridIdsMin = ivec2(_accelParticles->pointToClosestIndex(b.stretchedCornerLB));
                gridIdsMax = ivec2(_accelParticles->pointToClosestIndex(b.stretchedCornerRT));
            }
            else {
                vec2 lb = b.stretchedCornerLB;
                vec2 lt = b.stretchedCornerLT;
                vec2 rb = b.stretchedCornerRB;
                vec2 rt = b.stretchedCornerRT;
                float minX = min4(lb.x, lt.x, rb.x, rt.x);
                float maxX = max4(lb.x, lt.x, rb.x, rt.x);
                float minY = min4(lb.y, lt.y, rb.y, rt.y);
                float maxY = max4(lb.y, lt.y, rb.y, rt.y);
                gridIdsMin = ivec2(_accelParticles->pointToClosestIndex(vec2(minX, minY)));
                gridIdsMax = ivec2(_accelParticles->pointToClosestIndex(vec2(maxX, maxY)));
            }

            for (int i = gridIdsMin.x; i <= gridIdsMax.x; i++) {
                for (int j = gridIdsMin.y; j <= gridIdsMax.y; j++) {
                    vector<unsigned int>* partIds = _accelParticles->getCpuData_noRefresh(i, j);

                    for (auto partIt = partIds->begin(); partIt != partIds->end(); partIt++) {
                        vec2 p = particlesPointer[*partIt];
                        if (AllBitsSet(b.bitFlags, INTERIOR)) {
                            partVecsPointer[*partIt] += VecObstacle_stretch(p, b); // TODO: This is the slow part, optimize it.
                        }
                        partVecsPointer[*partIt] += b.coeffBoundary *
                            TranslatedBasisEval(  // TODO: This is the slow part, optimize it.
                                p, b.freqLvl, b.center
                            )
                            ;


                    }
                }
            }
        }


        // actually advect particle
        for (unsigned int iPart = 0; iPart < _partPos->_nbElements; ++iPart) {
            vec2 vec = partVecsPointer[iPart];
            particlesPointer[iPart] += _dt / _substepsParticles * vec;
            partAgesPointer[iPart] += _dt / _substepsParticles;

        }


        if (pushParticlesOutOfObstacles)
        {
            // move particles out of obstacles
            for (unsigned int iPart = 0; iPart < _partPos->_nbElements; ++iPart)
            {
                vec2& p = particlesPointer[iPart];
                for (Obstacle* obs : _obstacles) {
                    if (obs->phi(p) < 0)
                    {
                        p -= obs->gradPhi(p) * obs->phi(p);
                    }
                }
            }
        }

    }


    _partPos->_sourceStorageType = DataBuffer1D<vec2>::StorageType::CPU;
    _partVecs->_sourceStorageType = DataBuffer1D<vec2>::StorageType::CPU;
    _partAges->_sourceStorageType = DataBuffer1D<float>::StorageType::CPU;

}






void Application::SeedParticles()
{
    bool pushParticlesOutOfObstacles = true;


    // reset seed cursor at beginning of circular buffer after loop, and detect looping to switch from appending to replacing
    unsigned int nbTotalPartSeed = MAX_NB_PARTICLE_SEED_GROUPS * NB_PARTICLES_TO_SEED_PER_DIM;
    if (_particleCircularSeedId >= nbTotalPartSeed) {
        _particleSeedBufferLooped = true;
        _particleCircularSeedId = 0;
    }




    int seedingDensity = int(NB_PARTICLES_TO_SEED_PER_DIM);


    for (int i = 0; i < seedingDensity; ++i) {
        // random seeding
        vec2 p = vec2(_seedCenterX, _seedCenterY) + glm::diskRand(_seedRadius);
        bool isInsideObstacle = false;
        for (Obstacle* obs : _obstacles) {
            if (obs->phi(p) <= 0) {
                isInsideObstacle = true;
                break;
            }
        }
        if (pushParticlesOutOfObstacles && isInsideObstacle) { continue; }
        float temp = mod(float(10.f*(p.y - _domainBottom) / (_domainTop - _domainBottom)), 1.f);
        //vec3 c = vec3(1-temp,0.5*(1-temp),0.8);
        vec3 c = vec3(0, 0, 0);


        if (_particleSeedBufferLooped) {
            _partPos->setCpuData(_particleCircularSeedId, p);
            _partVecs->setCpuData(_particleCircularSeedId, vec2(0));
            //_colors->setCpuData(_particleCircularSeedId, c);
            _partAges->setCpuData(_particleCircularSeedId, 0);
        }
        else {
            _partPos->appendCpu(p);
            _partVecs->appendCpu(vec2(0));
            //_colors->appendCpu(c);
            _partAges->appendCpu(0);
        }
        _particleCircularSeedId++;

    }

}



