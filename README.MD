This engine is an attempt to update the Tesseract Engine

Todo:
- merge changes from the imprimis/libprimis projects (they have already done a lot)

- `find src/ -name '*.cpp' | xargs -I{} clang-tidy {} -p build -fix
 -checks='modernize-*,readability-*,-modernize-pass-by-value'`

- manually modernize the code and architecture to make it more modular, testable and SOLID

- Switch to SDL3: https://github.com/libsdl-org/SDL/blob/main/docs/README-migration.md

- migrate cube script to luascript: 
    - https://github.com/jordanvrtanoski/luacpp
    - https://www.geeksforgeeks.org/integrating-lua-in-cpp/
- use AI to translate old cube scripts

- migrate renderer to Vulkan:
    - temporary solution: https://github.com/kbiElude/VKGL
    - https://www.youtube.com/watch?v=Y9U9IE0gVHA&list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR
    - https://my.eng.utah.edu/~cs5610/lectures/Migrating_from_OpenGL_to_Vulkan.pdf
    - https://developer.nvidia.com/transitioning-opengl-vulkan
    - https://www.reddit.com/r/vulkan/comments/krr77b/resources_for_rewriting_an_opengl11_app_to_vulkan/
    - https://vkguide.dev/
    - https://vulkan-tutorial.com/resources/vulkan_tutorial_en.pdf
- switch from custom md5 model loader to assimp or anything that allows gltf support:
    - https://bevy-cheatbook.github.io/3d/gltf.html
    - https://github.com/code4game/libgltf
    - https://github.com/syoyo/tinygltf
    - https://github.com/microsoft/glTF-SDK


General useful resources for game engine development:
- https://wiki.installgentoo.com/wiki//gedg/_-_Game_and_Engine_Dev_General