/* AsyncHandlerTread - Thread that communicates with an async device.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */


package com.salo.android.arduinointegration;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.google.android.things.pio.PeripheralManager;
import com.google.android.things.pio.UartDevice;
import com.google.android.things.pio.UartDeviceCallback;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;
import java.util.List;



public class AsyncHandlerThread extends HandlerThread {

    private MainActivity mActivity;     // activity
    private String mAsyncDevice;        // name of async device to use
    private UartDevice mDevice;         // async device
    private Handler mHandler;
    private int sequence;               // sequence number for hello/ack

    private String TAG = AsyncHandlerThread.class.getSimpleName();


    /* AsyncHandlerThread(String) - constructor. */

    AsyncHandlerThread(MainActivity activity, String device) {
        super(device);
        mActivity = activity;
        mAsyncDevice = device;
        Log.d(TAG, "AsyncHandlerThread(" + mAsyncDevice + ")");
    }



    /* run() - main loop.
     */

    @Override
    public void run() {
        Log.d(TAG, "run(" + mAsyncDevice + ")");

        /* List available async devices. */

        PeripheralManager manager = PeripheralManager.getInstance();
        List<String> deviceList = manager.getUartDeviceList();
        if (deviceList.isEmpty()) {
            Log.i(TAG, "No available async device.");
            return;
        } else {
            Log.i(TAG, "Available async devices: " + deviceList);
        }

        /* Try to acquire async device. */

        try {
            mDevice = manager.openUartDevice(mAsyncDevice);
            Log.d(TAG, "Acquired async Device: " + mAsyncDevice);
        } catch (IOException e) {
            Log.w(TAG, "Unable to acquire async device: " + mAsyncDevice, e);
            return;
        }

        /* Configure async device. */

        if (mDevice != null) {
            try {
                mDevice.setBaudrate(57600);
                mDevice.setDataSize(8);
                mDevice.setParity(UartDevice.PARITY_NONE);
                mDevice.setStopBits(1);
                mDevice.setHardwareFlowControl(UartDevice.HW_FLOW_CONTROL_NONE);
            } catch (IOException e) {
                Log.d(TAG, "Unable to configure async device: " + mAsyncDevice);
                e.printStackTrace();
                return;
            }
            Log.d(TAG, "Async device configured: " + mAsyncDevice);
        }

        /* Register UartDeviceCallback. */

        UartDeviceCallback mUartCallback = new UartDeviceCallback() {
            @Override
            public boolean onUartDeviceDataAvailable(UartDevice uart) {
//                Log.d(TAG, "onUartDeviceDataAvailable: " + mAsyncDevice);
                // Read available data from the UART device
                try {
                    readUartBuffer(uart);
                } catch (IOException e) {
                    Log.d(TAG, "Unable to access UART device: " + uart.getName(), e);
                }
                return true;            // listen for more interrupts
            }

            @Override
            public void onUartDeviceError(UartDevice uart, int error) {
                Log.w(TAG, uart + ": Error event " + error);
            }
        };

        Looper.prepare();               // get a Looper for this Thread

        try {
            mDevice.registerUartDeviceCallback(mUartCallback);
        } catch (IOException e) {
            e.printStackTrace();
        }

        mHandler = new Handler() {
            public void handleMessage(Message msg) {
//                Log.d(TAG, "handleMessage(): " + msg.toString());
                int count;
                byte [] b;

//                while (true) {
                    try {
                        String helloSequence = String.format("hello: %05d\r\n", sequence++);
                        b = helloSequence.getBytes("UTF-8");
                        count = mDevice.write(b, b.length);
                    } catch (UnsupportedEncodingException e) {
                        e.printStackTrace();
                    } catch (IOException e) {
                        e.fillInStackTrace();
                    }

                    sendEmptyMessageDelayed(999, 5000);
            }
        };

        mHandler.sendEmptyMessage(999); // queue up code to xmit "hello" msgs

        Log.d(TAG, "Done registering async device callback: " + mAsyncDevice);

        Looper.loop();                  // start Looper

        Log.d(TAG, "Looper.loop() ****** Should never get here? ******");

        /* Clean up on termination.  Note: will never get here. */

//        if (mDevice1 != null) {
//            try {
//                mDevice1.close();
//                mDevice1 = null;
//            } catch (IOException e) {
//                Log.w(TAG, "Unable to close UART device", e);
//            }
//        }

    }



    /* readUartBuffer - read data from uart buffer.
     */

    private void readUartBuffer(UartDevice uart) throws IOException {

        final int maxCount = 100;        // max data to read at once
        byte[] buffer = new byte[maxCount];    // data buffer
        double temp = 0.0;              // temp read over async interface

        int count;
        while ((count = uart.read(buffer, buffer.length)) > 0) {
            String string = new String(buffer, 0, count, StandardCharsets.UTF_8);
            String tokens[] = string.split("[ ]+");
            if (tokens.length >= 2) {

                /* Process "temp: <temp>" command. */

                if (tokens[0].equalsIgnoreCase("temp:")) {
                    try
                    {
                        temp = Double.valueOf(tokens[1]);
                        mActivity.setAsyncTemp(temp);
                    }
                    catch (NumberFormatException nfe)
                    {
                        mActivity.setAsyncTemp(-3.0);
                    }
                }
            }
            Log.d(TAG, "Rx async: " + string.replace("\n", ""));
        }
    }

}
