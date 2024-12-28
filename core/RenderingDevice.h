#pragma once

#include "buffer/Buffer.h"
#include "buffer/BufferUsage.h"
#include "image/Image.h"
#include "image/ImageFormat.h"
#include "image/ImageView.h"
#include "image/Sampler.h"
#include "image/SamplerState.h"

namespace Vixen {
    class RenderingDevice {
    public:
        virtual ~RenderingDevice() = default;

        /****************/
        /**** BUFFER ****/
        /****************/

        virtual Buffer *createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) = 0;

        virtual void destroyBuffer(Buffer *buffer) = 0;

        /*****************/
        /***** IMAGE *****/
        /*****************/

        virtual Image *createImage(const ImageFormat &format, const ImageView &view) = 0;

        virtual std::byte *mapImage(Image *image) = 0;

        virtual void unmapImage(Image *image) = 0;

        virtual void destroyImage(Image *image) = 0;

        /*****************/
        /**** SAMPLER ****/
        /*****************/

        virtual Sampler *createSampler(SamplerState state) = 0;

        virtual void destroySampler(Sampler *sampler) = 0;
    };
}
