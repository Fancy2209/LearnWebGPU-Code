/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2024 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

 #include <webgpu/webgpu.h>

 #ifdef __EMSCRIPTEN__
 #  include <emscripten.h>
 #endif // __EMSCRIPTEN__
 
 #include <iostream>
 #include <cassert>
 #include <vector>
 
 /**
  * Utility function to get a WebGPU adapter, so that
  *     WGPUAdapter adapter = requestAdapterSync(options);
  * is roughly equivalent to
  *     const adapter = await navigator.gpu.requestAdapter(options);
  */
 WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
     // A simple structure holding the local information shared with the
     // onAdapterRequestEnded callback.
     struct UserData {
         WGPUAdapter adapter = nullptr;
         bool requestEnded = false;
     };
     UserData userData;
 
     // Callback called by wgpuInstanceRequestAdapter when the request returns
     // This is a C++ lambda function, but could be any function defined in the
     // global scope. It must be non-capturing (the brackets [] are empty) so
     // that it behaves like a regular C function pointer, which is what
     // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
     // is to convey what we want to capture through one of the userData pointers,
     // provided as the last two arguments of wgpuInstanceRequestAdapter and received
     // by the callback as its last 2 arguments.
     auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void * userData1, void *) {
         UserData& userData = *reinterpret_cast<UserData*>(userData1);
         if (status == WGPURequestAdapterStatus_Success) {
             userData.adapter = adapter;
         } else {
             std::cout << "Could not get WebGPU adapter: " << message.data << std::endl;
         }
         userData.requestEnded = true;
     };
 
     // Information about the callback called by wgpuInstanceRequestAdapter, this is
     // where we set what the callback function is and our userdata
     WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
     adapterCallbackInfo.callback = onAdapterRequestEnded;
     adapterCallbackInfo.userdata1 = (void *)&userData;
 
     // Call to the WebGPU request adapter procedure
     wgpuInstanceRequestAdapter(
         instance /* equivalent of navigator.gpu */,
         options,
         adapterCallbackInfo
     );
 
     // We wait until userData.requestEnded gets true
 #ifdef __EMSCRIPTEN__
         while (!userData.requestEnded) {
             emscripten_sleep(100);
         }
 #endif // __EMSCRIPTEN__
 
     assert(userData.requestEnded);
 
     return userData.adapter;
 }
 
 void inspectAdapter(WGPUAdapter adapter) {
 #ifndef __EMSCRIPTEN__
     WGPULimits supportedLimits = {};
     supportedLimits.nextInChain = nullptr;
 
     bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success; 
     if (success) {
         std::cout << "Adapter limits:" << std::endl;
         std::cout << " - maxTextureDimension1D: " << supportedLimits.maxTextureDimension1D << std::endl;
         std::cout << " - maxTextureDimension2D: " << supportedLimits.maxTextureDimension2D << std::endl;
         std::cout << " - maxTextureDimension3D: " << supportedLimits.maxTextureDimension3D << std::endl;
         std::cout << " - maxTextureArrayLayers: " << supportedLimits.maxTextureArrayLayers << std::endl;
     }
 #endif // NOT __EMSCRIPTEN__
     WGPUSupportedFeatures features;
     wgpuAdapterGetFeatures(adapter, &features);
 
     std::cout << "Adapter features:" << std::endl;
     std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
     for (int i = 0; i < features.featureCount; i++) {
         std::cout << " - 0x" << features.features[i] << std::endl;
     }
     std::cout << std::dec; // Restore decimal numbers
     WGPUAdapterInfo info = {};
     info.nextInChain = nullptr;
     wgpuAdapterGetInfo(adapter, &info);
     std::cout << "Adapter info:" << std::endl;
     std::cout << " - vendorID: " << info.vendorID << std::endl;
     if (info.vendor.data) {
         std::cout << " - vendorName: " << info.vendor.data << std::endl;
     }
     if (info.architecture.data) {
         std::cout << " - architecture: " << info.architecture.data << std::endl;
     }
     std::cout << " - deviceID: " << info.deviceID << std::endl;
     if (info.device.data) {
         std::cout << " - device: " << info.device.data << std::endl;
     }
     if (info.description.data) {
         std::cout << " - driverDescription: " << info.description.data << std::endl;
     }
     std::cout << std::hex;
     std::cout << " - adapterType: 0x" << info.adapterType << std::endl;
     std::cout << " - backendType: 0x" << info.backendType << std::endl;
     std::cout << std::dec; // Restore decimal numbers
 }
 
 int main() {
     WGPUInstanceDescriptor desc = {};
     desc.nextInChain = nullptr;
 
 #ifdef WEBGPU_BACKEND_EMSCRIPTEN
     WGPUInstance instance = wgpuCreateInstance(nullptr);
 #else //  WEBGPU_BACKEND_EMSCRIPTEN
     WGPUInstance instance = wgpuCreateInstance(&desc);
 #endif //  WEBGPU_BACKEND_EMSCRIPTEN
 
     if (!instance) {
         std::cerr << "Could not initialize WebGPU!" << std::endl;
         return 1;
     }
 
     std::cout << "WGPU instance: " << instance << std::endl;
 
     std::cout << "Requesting adapter..." << std::endl;
     WGPURequestAdapterOptions adapterOpts = {};
     adapterOpts.nextInChain = nullptr;
     WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
     std::cout << "Got adapter: " << adapter << std::endl;
 
     // Display some information about the adapter
     inspectAdapter(adapter);
 
     // We no longer need to use the instance once we have the adapter
     wgpuInstanceRelease(instance);
 
     wgpuAdapterRelease(adapter);
 
     return 0;
 }
 