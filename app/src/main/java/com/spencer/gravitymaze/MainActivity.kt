package com.spencer.gravitymaze

import android.view.View
import android.content.Context;
import android.os.Handler
import android.os.Looper
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiDevice;
import android.media.midi.MidiManager;
import com.google.androidgamesdk.GameActivity

class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("gravitymaze")
        }
    }

    fun openMidiDevice(): MidiDevice? {
        val midiManager = this.getSystemService(Context.MIDI_SERVICE) as MidiManager
        //val deviceInfoArray = midiManager.getDevicesForTransport(MidiManager.TRANSPORT_MIDI_BYTE_STREAM)
        val deviceInfoArray = midiManager.devices
        var openedDevice: MidiDevice? = null
        for(deviceInfo in deviceInfoArray) {
            if(deviceInfo.getOutputPortCount() > 0) {
                midiManager.openDevice(deviceInfo, { device ->
                    openedDevice = device
                }, Handler(Looper.getMainLooper()))
                break
            }
        }

        return openedDevice
    }

    fun closeMidiDevice(openedDevice: MidiDevice) {
        //val midiManager = this.getSystemService(Context.MIDI_SERVICE) as MidiManager
        openedDevice.close();
    }

    fun gameActivityFinished() {
        this.finish()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            hideSystemUi()
        }
    }

    private fun hideSystemUi() {
        val decorView = window.decorView
        decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }
}