package com.example.projectm_texture

import android.view.Surface
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.MethodCallHandler
import io.flutter.plugin.common.MethodChannel.Result
import io.flutter.view.TextureRegistry

class ProjectmTexturePlugin : FlutterPlugin, MethodCallHandler {
    private lateinit var channel: MethodChannel
    private var textureRegistry: TextureRegistry? = null
    
    init {
        try {
            System.loadLibrary("projectm_ffi")
        } catch (e: UnsatisfiedLinkError) {
        }
    }

    private external fun nativeSetSurface(surface: Surface?, width: Int, height: Int)

    override fun onAttachedToEngine(flutterPluginBinding: FlutterPlugin.FlutterPluginBinding) {
        textureRegistry = flutterPluginBinding.textureRegistry
        channel = MethodChannel(flutterPluginBinding.binaryMessenger, "projectm_texture")
        channel.setMethodCallHandler(this)
    }

    override fun onMethodCall(call: MethodCall, result: Result) {
        when (call.method) {
            "initialize" -> {
                val registry = textureRegistry
                if (registry == null) {
                    result.error("UNAVAILABLE", "TextureRegistry not available", null)
                    return
                }
                
                val args = call.arguments as? Map<*, *>
                val width = args?.get("width") as? Int ?: 800
                val height = args?.get("height") as? Int ?: 600

                val entry = registry.createSurfaceTexture()
                val surfaceTexture = entry.surfaceTexture()
                surfaceTexture.setDefaultBufferSize(width, height)
                
                val surface = Surface(surfaceTexture)
                
                try {
                    System.loadLibrary("projectm_ffi")
                    nativeSetSurface(surface, width, height)
                } catch (e: Exception) {
                    result.error("JNI_ERROR", "Failed to pass surface to C++: ${e.message}", null)
                    return
                }

                result.success(entry.id())
            }
            "destroyTexture" -> {
                try {
                    nativeSetSurface(null, 0, 0)
                } catch (e: Exception) {
                }
                result.success(null)
            }
            else -> result.notImplemented()
        }
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        channel.setMethodCallHandler(null)
        textureRegistry = null
    }
}
