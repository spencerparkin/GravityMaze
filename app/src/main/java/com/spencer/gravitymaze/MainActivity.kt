package com.spencer.gravitymaze

import android.view.View
import android.content.Context;
import android.os.Handler
import android.os.Looper
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiDevice;
import android.media.midi.MidiManager;
import android.os.Bundle
import android.os.PersistableBundle
import com.google.androidgamesdk.GameActivity

class Listener : MidiManager.OnDeviceOpenedListener {
    var openedDevice: MidiDevice? = null
    override fun onDeviceOpened(device: MidiDevice?) {
        this.openedDevice = device
    }
}

class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("gravitymaze")
        }
    }

    var listener = Listener()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val midiManager = this.getSystemService(Context.MIDI_SERVICE) as MidiManager
        val deviceInfoArray = midiManager.getDevices()
        //val deviceInfoArray: Array<MidiDeviceInfo> = midiManager.getDevicesForType(MidiDeviceInfo.TYPE_SYNTHESIZER)
        //var deviceInfoArray = midiManager.getDevicesForTransport(MidiManager.TRANSPORT_MIDI_BYTE_STREAM)
        for(deviceInfo in deviceInfoArray) {
            val portInfos = deviceInfo.ports
            for (portInfo in portInfos) {
                if (portInfo.type == MidiDeviceInfo.PortInfo.TYPE_INPUT) {
                    Log.d("MIDI", "Input port found: " + portInfo.name)
                }
            }

            // TODO: This won't work.  We need an input port.
            if(deviceInfo.getOutputPortCount() > 0) {
                midiManager.openDevice(deviceInfo, this.listener, Handler(Looper.getMainLooper()))
                break
            }
        }
    }

    fun getOpenedMidiDevice():MidiDevice? {
        return this.listener.openedDevice
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