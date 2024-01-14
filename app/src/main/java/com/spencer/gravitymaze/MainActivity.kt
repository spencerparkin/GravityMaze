package com.spencer.gravitymaze

import android.view.View
import android.content.Context;
import android.os.Handler
import android.os.Looper
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiDevice;
import android.media.midi.MidiManager;
import android.os.Bundle
import android.util.Log
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

    fun logMidiDevice(deviceInfo: MidiDeviceInfo) {
        val bundle: Bundle = deviceInfo.getProperties()
        val manufacturer: String? = bundle.getString(MidiDeviceInfo.PROPERTY_MANUFACTURER)
        val name: String? = bundle.getString(MidiDeviceInfo.PROPERTY_NAME)
        val product: String? = bundle.getString(MidiDeviceInfo.PROPERTY_SERIAL_NUMBER)
        val serial: String? = bundle.getString(MidiDeviceInfo.PROPERTY_SERIAL_NUMBER)
        val version: String? = bundle.getString(MidiDeviceInfo.PROPERTY_VERSION)
        Log.d("logMidiDevice", "=============================================")
        Log.d("logMidiDevice", "Manufacturer: " + manufacturer)
        Log.d("logMidiDevice", "Name:         " + name)
        Log.d("logMidiDevice", "Product:      " + product)
        Log.d("logMidiDevice", "Serial:       " + serial)
        Log.d("logMidiDevice", "Version:      " + version)

        val portInfoArray: Array<MidiDeviceInfo.PortInfo> = deviceInfo.getPorts()
        Log.d("logMidiDevice", "Num ports:    " + portInfoArray.size)
        for(portInfo in portInfoArray) {
            Log.d("logMidiDevice", "------------------------------")
            val portType = portInfo.getType()
            val portName = portInfo.getName()
            val portNumber = portInfo.getPortNumber()
            Log.d("logMidiDevice", "Port #: " + portNumber)
            Log.d("logMidiDevice", "Name  : " + portName)
            Log.d("logMidiDevice", "Type  : " + portType)
        }
    }

    fun kickOffMidiDeviceOpen(): Boolean {
        val midiManager = this.getSystemService(Context.MIDI_SERVICE) as MidiManager

        // Can't use this, because my phone is too old.
        //val deviceInfoArray: Array<MidiDeviceInfo> = midiManager.getDevicesForType(MidiDeviceInfo.TYPE_SYNTHESIZER)

        // Can't use this, because it's deprecated, I think.
        //var deviceInfoArray = midiManager.getDevicesForTransport(MidiManager.TRANSPORT_MIDI_BYTE_STREAM)

        // Fortunately, this still seems to work.
        val deviceInfoArray = midiManager.getDevices()

        // Log everything we find for starters.
        for(deviceInfo in deviceInfoArray) {
            this.logMidiDevice(deviceInfo)
        }

        // First, look for a device with an input port.  (The device
        // receives input through the port from us.)
        var foundDeviceInfo: MidiDeviceInfo? = null
        for(deviceInfo in deviceInfoArray) {
            if(deviceInfo.getInputPortCount() > 0) {
                // This doesn't seem like a good stratagy.
                // What I really need to know is if the device can synthasize.
                val bundle: Bundle = deviceInfo.getProperties()
                val name: String? = bundle.getString(MidiDeviceInfo.PROPERTY_NAME)
                if(name != null && name.contains("Player")) {
                    foundDeviceInfo = deviceInfo
                }
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
            Log.d("kickOffMidiDeviceOpen", "CHOSEN DEVICE FOLLOWS...")
            this.logMidiDevice(foundDeviceInfo)
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