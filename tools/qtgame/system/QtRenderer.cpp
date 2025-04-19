#include "QtRenderer.h"

#include <rhi/qrhi.h>

#include"../../../a51lib/Bitmap.h"

QtRenderer::QtRenderer()
{
    batch = nullptr;
}

void QtRenderer::startFrame(QRhiResourceUpdateBatch *batch, QRhi* rhi)
{
    this->batch = batch;
    this->rhi = rhi;
}

QRhiResourceUpdateBatch * QtRenderer::endFrame()
{
    auto xbatch = batch;
    batch = nullptr;
    return xbatch;
}

void QtRenderer::drawBegin(Primitive, int drawFlags)
{
}

void QtRenderer::drawEnd()
{
}

void QtRenderer::setTexture(Bitmap* tex)
{
    // TODO: What is the lifetime of this? Do we need to track and release it or does RHI take care of that?
    QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(tex->width, tex->height));
    if (!texture->create()) { return; }
    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    tex->convertFormat(Bitmap::FMT_32_ARGB_8888);
    QImage image = QImage((const uint8_t*)tex->getPixelData(0), tex->width, tex->height, tex->physicalWidth * 4, QImage::Format_ARGB32);

    batch->uploadTexture(texture, image);
}

void QtRenderer::drawColour(const Colour& colour)
{
}

void QtRenderer::drawVertex(float x, float y, float z, float u, float v)
{
}
