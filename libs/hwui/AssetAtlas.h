/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HWUI_ASSET_ATLAS_H
#define ANDROID_HWUI_ASSET_ATLAS_H

#include "Texture.h"
#include "UvMapper.h"

#include <cutils/compiler.h>
#include <GLES2/gl2.h>
#include <ui/GraphicBuffer.h>
#include <SkBitmap.h>

#include <memory>
#include <unordered_map>

namespace android {
namespace uirenderer {

class Caches;
class Image;

/**
 * An asset atlas holds a collection of framework bitmaps in a single OpenGL
 * texture. Each bitmap is associated with a location, defined in pixels,
 * inside the atlas. The atlas is generated by the framework and bound as
 * an external texture using the EGLImageKHR extension.
 */
class AssetAtlas {
public:
    /**
     * Entry representing the texture and uvMapper of a PixelRef in the
     * atlas
     */
    class Entry {
    public:
        /*
         * A "virtual texture" object that represents the texture
         * this entry belongs to. This texture should never be
         * modified.
         */
        Texture* texture;

        /**
         * Maps texture coordinates in the [0..1] range into the
         * correct range to sample this entry from the atlas.
         */
        const UvMapper uvMapper;

        /**
         * Unique identifier used to merge bitmaps and 9-patches stored
         * in the atlas.
         */
        const void* getMergeId() const {
            return texture->blend ? &atlas.mBlendKey : &atlas.mOpaqueKey;
        }

        ~Entry() {
            delete texture;
        }

    private:
        /**
         * The pixel ref that generated this atlas entry.
         */
        SkPixelRef* pixelRef;

        /**
         * Atlas this entry belongs to.
         */
        const AssetAtlas& atlas;

        Entry(SkPixelRef* pixelRef, Texture* texture, const UvMapper& mapper,
                    const AssetAtlas& atlas)
                : texture(texture)
                , uvMapper(mapper)
                , pixelRef(pixelRef)
                , atlas(atlas) {
        }

        friend class AssetAtlas;
    };

    AssetAtlas(): mTexture(nullptr), mImage(nullptr),
            mBlendKey(true), mOpaqueKey(false) { }
    ~AssetAtlas() { terminate(); }

    /**
     * Initializes the atlas with the specified buffer and
     * map. The buffer is a gralloc'd texture that will be
     * used as an EGLImage. The map is a list of SkBitmap*
     * and their (x, y) positions
     *
     * This method returns immediately if the atlas is already
     * initialized. To re-initialize the atlas, you must
     * first call terminate().
     */
    ANDROID_API void init(const sp<GraphicBuffer>& buffer, int64_t* map, int count);

    /**
     * Destroys the atlas texture. This object can be
     * re-initialized after calling this method.
     *
     * After calling this method, the width, height
     * and texture are set to 0.
     */
    void terminate();

    /**
     * Returns the width of this atlas in pixels.
     * Can return 0 if the atlas is not initialized.
     */
    uint32_t getWidth() const {
        return mTexture ? mTexture->width() : 0;
    }

    /**
     * Returns the height of this atlas in pixels.
     * Can return 0 if the atlas is not initialized.
     */
    uint32_t getHeight() const {
        return mTexture ? mTexture->height() : 0;
    }

    /**
     * Returns the OpenGL name of the texture backing this atlas.
     * Can return 0 if the atlas is not initialized.
     */
    GLuint getTexture() const {
        return mTexture ? mTexture->id() : 0;
    }

    /**
     * Returns the entry in the atlas associated with the specified
     * pixelRef. If the pixelRef is not in the atlas, return NULL.
     */
    Entry* getEntry(const SkPixelRef* pixelRef) const;

    /**
     * Returns the texture for the atlas entry associated with the
     * specified pixelRef. If the pixelRef is not in the atlas, return NULL.
     */
    Texture* getEntryTexture(const SkPixelRef* pixelRef) const;

private:
    void createEntries(Caches& caches, int64_t* map, int count);

    Texture* mTexture;
    Image* mImage;

    const bool mBlendKey;
    const bool mOpaqueKey;

    std::unordered_map<const SkPixelRef*, std::unique_ptr<Entry>> mEntries;
}; // class AssetAtlas

}; // namespace uirenderer
}; // namespace android

#endif // ANDROID_HWUI_ASSET_ATLAS_H
