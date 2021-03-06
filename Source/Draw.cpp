
#include "ObstacleShader.h"
#include "ParticleShader.h"
#include "VelocityArrowShader.h"
#include "Application.h"
#include "Obstacles.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>

using namespace std;

void Application::Draw()
{
    // update obstacle display
    if (_obstacleDisplayNeedsUpdating) {

        _obstacleLines->resize(0);

        for (Obstacle* obs : _obstacles) {

            int nbOffsets = 9;
            float thickness = 0.005f;

            for (int offset = -nbOffsets; offset <= nbOffsets; offset++) {

                // get obstacle boundary with marching square
                float dx = (_domainRight - _domainLeft) / _obstacleDisplayRes;
                float dy = (_domainTop - _domainBottom) / _obstacleDisplayRes;
                for (int i = -2; i <= int(_obstacleDisplayRes) + 1; i++) {
                    for (int j = -2; j <= int(_obstacleDisplayRes) + 1; j++) {

                        // bl = bottom left, tr = top right.
                        vec2 blPos = vec2(_domainLeft + i * dx, _domainBottom + j * dy);
                        float bl = thickness * float(offset) / float(nbOffsets) + obs->phi(blPos);
                        float br = thickness * float(offset) / float(nbOffsets) + obs->phi(blPos + vec2(dx, 0));
                        float tl = thickness * float(offset) / float(nbOffsets) + obs->phi(blPos + vec2(0, dy));
                        float tr = thickness * float(offset) / float(nbOffsets) + obs->phi(blPos + vec2(dx, dy));

                        short flags = 8 * (bl >= 0 ? 1 : 0) + 4 * (br >= 0 ? 1 : 0) + 2 * (tl >= 0 ? 1 : 0) + 1 * (tr >= 0 ? 1 : 0);
                        switch (flags) {
                        case 0:
                        case 15:
                            break;
                        case 8:
                        case 7:
                            _obstacleLines->appendCpu(blPos + vec2(0, dy*RatioZero(bl, tl)));
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(bl, br), 0));
                            break;
                        case 4:
                        case 11:
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(bl, br), 0));
                            _obstacleLines->appendCpu(blPos + vec2(dx, dy*RatioZero(br, tr)));
                            break;
                        case 1:
                        case 14:
                            _obstacleLines->appendCpu(blPos + vec2(dx, dy*RatioZero(br, tr)));
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(tl, tr), dy));
                            break;
                        case 2:
                        case 13:
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(tl, tr), dy));
                            _obstacleLines->appendCpu(blPos + vec2(0, dy*RatioZero(bl, tl)));
                            break;
                        case 12:
                        case 3:
                            _obstacleLines->appendCpu(blPos + vec2(0, dy*RatioZero(bl, tl)));
                            _obstacleLines->appendCpu(blPos + vec2(dx, dy*RatioZero(br, tr)));
                            break;
                        case 10:
                        case 5:
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(bl, br), 0));
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(tl, tr), dy));
                            break;
                        case 9:
                        case 6: // crossed case, arbitrary choice is made for line orientation.
                            _obstacleLines->appendCpu(blPos + vec2(0, dy*RatioZero(bl, tl)));
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(bl, br), 0));
                            _obstacleLines->appendCpu(blPos + vec2(dx, dy*RatioZero(br, tr)));
                            _obstacleLines->appendCpu(blPos + vec2(dx*RatioZero(tl, tr), dy));
                            break;
                        }
                    }
                }
            }
        }

        _obstacleDisplayNeedsUpdating = false;
    }

    // display all simulation elements
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (_obstacleLines->_metadataBuffer.nbElements > 0) {
        _obstacleLines->TransferDataCpuToBuffer();
        _pipelineObstacle->Execute();
    }

    if (_showVelocity) {

        if (_velocityGridNeedsUpdating) {
            ComputeVelocityGridForDisplay();
            _velocityGridNeedsUpdating = false;
        }

        memcpy(
            _bufferArrows->getCpuDataPointer(),
            _velocityField->_vectors.getCpuDataPointer(),
            _velocityField->nbElementsX()*_velocityField->nbElementsY() * sizeof(vec2));

        _bufferGridPoints->TransferDataCpuToBuffer();
        _bufferArrows->TransferDataCpuToBuffer();

        _pipelineVelocityArrow->Execute();
    }

    if (_drawParticles) {
        _partPos->TransferDataCpuToBuffer();
        _pipelineParticle->Execute();
    }

    if (_saveWindowToFile) {
        SaveWindowToFile();
    }

    glfwSwapBuffers(_glfwWindow);
}


// Used for display only
void Application::ComputeVelocityGridForDisplay()
{
    // clear velocity grid
    _velocityField->populateWithFunction([](float /*x*/, float /*y*/) {return vec2(0); });

    for (unsigned int i = 0; i < _basisFlowParams->_nbElements; ++i) {
        BasisFlow b = _basisFlowParams->getCpuData(i);

        _velocityField->addFunction(
            [&](float x, float y) {
            vec2 p = vec2(x, y);

            vec2 vec(0);
            if (AllBitsSet(b.bitFlags, INTERIOR)) {
                vec += VecObstacle_stretch(p, b);
            }

            vec += b.coeffBoundary * (vec2)(
                TranslatedBasisEval(p, b.freqLvl, b.center)
                );

            return vec;
        }
        );
    }
}


void Application::SaveWindowToFile() {

    static bool initialized = false;
    static std::vector<char> pixels = {};
    static std::vector<char> pixelsFlipped = {};

    if (!initialized) {
        filesystem::create_directory("Output");
        pixels.resize(4 * _windowWidth * _windowHeight);
        pixelsFlipped.resize(4 * _windowWidth * _windowHeight);
        initialized = true;
    }

    stringstream filename;
    filename.clear();
    filename << "Output/" << "frame" << setfill('0') << setw(5) << _appFrameCount << ".png";

    glReadPixels(0, 0, _windowWidth, _windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // flip image vertically
    for (int j = 0; j < int(_windowHeight); j++) {
        for (int i = 0; i < 4 * int(_windowWidth); i++) {
            pixelsFlipped[4 * _windowWidth*j + i] =
                pixels[4 * _windowWidth*(_windowHeight - j - 1) + i];
        }
    }

    bool success = stbi_write_png(
        filename.str().c_str(), _windowWidth, _windowHeight, 4, pixelsFlipped.data(), 4 * _windowWidth);

    if (success) {
        std::cout << "Frame saved to file '" << filename.str() << "'." << std::endl;
    }
    else {
        std::cout << "Saving frame to file failed." << std::endl;
    }
}