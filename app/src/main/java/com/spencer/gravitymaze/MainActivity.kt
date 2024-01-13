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

class MidiDeviceOpenListener : MidiManager.OnDeviceOpenedListener {
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

    var midiDeviceOpenListener = MidiDeviceOpenListener()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        //...
    }

    fun kickOffMidiDeviceOpen(): Boolean {
        val midiManager = this.getSystemService(Context.MIDI_SERVICE) as MidiManager

        // Can't use this, because my phone is too old.
        //val deviceInfoArray: Array<MidiDeviceInfo> = midiManager.getDevicesForType(MidiDeviceInfo.TYPE_SYNTHESIZER)

        // Can't use this, because it's deprecated, I think.
        //var deviceInfoArray = midiManager.getDevicesForTransport(MidiManager.TRANSPORT_MIDI_BYTE_STREAM)

        // Fortunately, this still seems to work.
        val deviceInfoArray = midiManager.getDevices()

        // First, look for a device with an input port.  (We "input" into the port.)
        var foundDeviceInfo: MidiDeviceInfo? = null
        for(deviceInfo in deviceInfoArray) {
            if(deviceInfo.getInputPortCount() > 0) {
                foundDeviceInfo = deviceInfo
                break
            }
        }

        // If we didn't find it, then look for a device with an output port.
        // Sometimes the Android SDK misreports the capabilities of a device.
        // That is, even if a device only has an output port, we can still open
        // an input port on the device.  Thanks, Google.
        if(foundDeviceInfo == null) {
            for (deviceInfo in deviceInfoArray) {
                if (deviceInfo.getOutputPortCount() > 0) {
                    foundDeviceInfo = deviceInfo
                    break
                }
            }
        }

        // Finally, if we found one, kick off the open.  This is asynchronous,
        // so it's not like we can return the opened device from this call.
        // Rather, the calling code will check back with us periodically to
        // see if the open actually succeeded.
        if(foundDeviceInfo != null) {
            midiManager.openDevice(foundDeviceInfo, this.midiDeviceOpenListener, Handler(Looper.getMainLooper()))
            return true
        }

        // Indicate that no open was kicked-off.
        return false
    }

    // This function will get called from the C++ side of things periodically
    // until we have something non-null to return here.
    fun getOpenedMidiDevice():MidiDevice? {
        return this.midiDeviceOpenListener.openedDevice
    }

    // Amazingly, there is no way to finish an activity from the C++ side of
    // things, so this method has to be called using the JNI.
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