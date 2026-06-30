package com.example.visual_music

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioPlaybackCaptureConfiguration
import android.media.AudioRecord
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.os.Bundle
import androidx.annotation.NonNull
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel
import kotlin.concurrent.thread

class MainActivity : FlutterActivity() {
    private val METHOD_CHANNEL = "com.example.visual_music/audio_control"
    private val EVENT_CHANNEL = "com.example.visual_music/audio_data"

    private var audioRecord: AudioRecord? = null
    private var isRecording = false
    private var eventSink: EventChannel.EventSink? = null
    private lateinit var mediaProjectionManager: MediaProjectionManager
    private val REQUEST_MEDIA_PROJECTION = 1001

    override fun configureFlutterEngine(@NonNull flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)

        mediaProjectionManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager

        EventChannel(flutterEngine.dartExecutor.binaryMessenger, EVENT_CHANNEL).setStreamHandler(
            object : EventChannel.StreamHandler {
                override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
                    eventSink = events
                }
                override fun onCancel(arguments: Any?) {
                    eventSink = null
                }
            }
        )

        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, METHOD_CHANNEL).setMethodCallHandler { call, result ->
            when (call.method) {
                "startCapture" -> {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        startActivityForResult(mediaProjectionManager.createScreenCaptureIntent(), REQUEST_MEDIA_PROJECTION)
                        result.success(true)
                    } else {
                        // Fallback to microphone for older Android versions
                        startMicrophoneCapture()
                        result.success(true)
                    }
                }
                "stopCapture" -> {
                    stopAudioCapture()
                    result.success(true)
                }
                else -> {
                    result.notImplemented()
                }
            }
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_MEDIA_PROJECTION) {
            if (resultCode == RESULT_OK && data != null) {
                val mediaProjection = mediaProjectionManager.getMediaProjection(resultCode, data)
                startLoopbackCapture(mediaProjection)
            } else {
                // User denied permission, fallback to microphone
                startMicrophoneCapture()
            }
        }
    }

    private fun startLoopbackCapture(mediaProjection: MediaProjection) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            val config = AudioPlaybackCaptureConfiguration.Builder(mediaProjection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA)
                .addMatchingUsage(AudioAttributes.USAGE_GAME)
                .addMatchingUsage(AudioAttributes.USAGE_UNKNOWN)
                .build()

            val format = AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                .setSampleRate(44100)
                .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
                .build()

            val bufferSize = AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_FLOAT)

            audioRecord = AudioRecord.Builder()
                .setAudioFormat(format)
                .setBufferSizeInBytes(bufferSize)
                .setAudioPlaybackCaptureConfig(config)
                .build()

            startRecordingThread(bufferSize)
        }
    }

    private fun startMicrophoneCapture() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), 1002)
            return
        }

        val format = AudioFormat.Builder()
            .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
            .setSampleRate(44100)
            .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
            .build()

        val bufferSize = AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_FLOAT)

        audioRecord = AudioRecord.Builder()
            .setAudioSource(android.media.MediaRecorder.AudioSource.MIC)
            .setAudioFormat(format)
            .setBufferSizeInBytes(bufferSize)
            .build()

        startRecordingThread(bufferSize)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1002 && grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            startMicrophoneCapture()
        }
    }

    private fun startRecordingThread(bufferSize: Int) {
        audioRecord?.startRecording()
        isRecording = true

        thread {
            val floatBuffer = FloatArray(bufferSize / 4)
            while (isRecording) {
                val read = audioRecord?.read(floatBuffer, 0, floatBuffer.size, AudioRecord.READ_BLOCKING) ?: 0
                if (read > 0) {
                    runOnUiThread {
                        eventSink?.success(floatBuffer.copyOfRange(0, read))
                    }
                }
            }
        }
    }

    private fun stopAudioCapture() {
        isRecording = false
        audioRecord?.stop()
        audioRecord?.release()
        audioRecord = null
    }
}
