/* SpiHandlerThread - a thread that receives temperature sensor data from an
 * Arduino board over an SPI interface.
 *
 * Copyright (C) Timothy J. Salo, 2018-2019.
 */

package com.salo.android.arduinointegration;

import android.os.HandlerThread;
import android.util.Log;

import com.google.android.things.pio.I2cDevice;
import com.google.android.things.pio.PeripheralManager;
import com.google.android.things.pio.SpiDevice;
//import com.google.android.things.pio.PioException;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.Runnable;
import java.util.List;

import static com.salo.android.arduinointegration.SpiHandlerThread.State.HELLO_SENT;
import static com.salo.android.arduinointegration.SpiHandlerThread.State.IDLE;
import static com.salo.android.arduinointegration.SpiHandlerThread.State.LINK_ACTIVE;
import static com.salo.android.arduinointegration.SpiHandlerThread.State.LINK_ESTABLISHED;
import static com.salo.android.arduinointegration.SpiHandlerThread.State.SEND_SENT;

public class SpiHandlerThread extends HandlerThread {

    private static final String TAG = SpiHandlerThread.class.getSimpleName();

    private static final int REC_BUFF_LENG = 50;    // receive buffer length

    MainActivity activity;              // the Activity

    private String mSpiDeviceName;
    private SpiDevice mSpiDevice;

    enum State {IDLE, HELLO_SENT, LINK_ESTABLISHED, SEND_SENT, LINK_ACTIVE}
    private State state;

    private int sequence;               // sequence number for hello/ack
    private byte recBuff[] = new byte[REC_BUFF_LENG];    // receive buffer
    private int recBuffp;               // receive buffer pointer



    /* SpiHandlerThread()
     */

    SpiHandlerThread (MainActivity activity, String device) {
        super(device);
        this.activity = activity;
        mSpiDeviceName = device;
        recBuffp = 0;
        state = IDLE;
    }

    /* run()
     */

    public void run() {

        Log.d(TAG, "run()");

        PeripheralManager manager = PeripheralManager.getInstance();

        /* Find SPI devices. */

        List<String> deviceList = manager.getSpiBusList();
        if (deviceList.isEmpty()) {
            Log.i(TAG, "No SPI bus available on this device.");
        } else {
            Log.i(TAG, "List of available devices: " + deviceList);
        }

        /* Open SPI device. */

        try {
            mSpiDevice = manager.openSpiDevice(mSpiDeviceName);
        } catch (IOException e) {
            Log.w(TAG, "Unable to access SPI device", e);
        }

        Log.d(TAG, "Acquired SPI device: " + mSpiDevice);

        try {
            mSpiDevice.setCsChange(false);
            mSpiDevice.setBitJustification(SpiDevice.BIT_JUSTIFICATION_MSB_FIRST);
//            mSpiDevice.setBitJustification(SpiDevice.BIT_JUSTIFICATION_LSB_FIRST);
            mSpiDevice.setFrequency(122000);
            mSpiDevice.setFrequency(61000);
            mSpiDevice.setFrequency(7629);
            mSpiDevice.setMode(SpiDevice.MODE0);
            mSpiDevice.setDelay(1000);
        } catch (IOException e) {
            e.printStackTrace();
        }

        activity.setSPITemp(-2.0);

        /* Operate SPI link. */

        String helloSequence = null;    // save "hello <seq> command
        String[] tokens = null;         // tokens returned by String.scan()
        String[] tokens2 = null;        // tokens returned by another String.scan()

        while (true) {

            switch (state) {

                /* IDLE state: send "hello <seq>". */

                case IDLE:

                    /* Send "hello <seq>" messages to slave. */

                    try {
                        helloSequence = String.format("hello: %05d\n", sequence++);
                        byte[] data = helloSequence.getBytes("UTF-8");
                        mSpiDevice.write(data, data.length);
                        Log.d(TAG, "Tx: " + helloSequence);
                    } catch (IOException e) {
                        Log.d(TAG, "IDLE: write of \"hello\" failed.");
                    }

                    /* Give slave some time to get ready to transmit. */

                    try {
                        sleep(150);
                    } catch (InterruptedException e) {
                        Log.d(TAG, "IDLE: sleep interrupted.");
                    }

                    state = HELLO_SENT; // update state

                    break;

                /* state HELLO_SENT: try to receive "ack <seq>". */

                case HELLO_SENT:

                    /* Try to receive "ack <seq>" from the slave. */

                    try {
                        recBuff[0] = '\n';    // in case of empty read
                        mSpiDevice.read(recBuff, REC_BUFF_LENG);   // read from I2C bus
                    } catch (IOException e) {
                        Log.d(TAG, "HELLO_SENT: read ack from slave failed.");
                        try {
                            sleep(5000);
                        } catch (InterruptedException e1) {
                            Log.d(TAG, "HELLO_SENT: sleep interrupted");
                        }
                        state = IDLE;   // go back to sending acks
                        break;
                    }

                    /* Check to see if received message is an ack. */

                    String recString = "xxx";

                    recBuff[0] &= 0x7f;    // ****** HACK: ****** clear upper bit of first received byte

                    /* Terminate received message at first '\n' char. */

                    boolean found = false;

                    for (int i = 0; i < recBuff.length; i++) {
                        if (recBuff[i] == '\n') {
                            recBuff[i] = 0;
                            found = true;
                            Log.d(TAG, "xxx i:" + i);
                            try {
                                recString = new String(recBuff, 0, i, "UTF-8");
                            } catch (UnsupportedEncodingException e) {
                                Log.d(TAG, "HELLO_SENT: UnsupportedEncodingException.");
                            }
                            break;      // is this right???
                        }
                    }

                    if (!found) {
                        Log.d(TAG, String.format("not found %02x, %02x, %02x", recBuff[0], recBuff[1], recBuff[2]));
                        try {
                            recString = new String(recBuff, 0, recBuff.length, "UTF-8");
                        } catch (UnsupportedEncodingException e) {
                            Log.d(TAG, "HELLO_SENT: UnsupportedEncodingException.");
                        }
                    }

                    Log.d(TAG, "Rx: " + recString);

                    /* Check for a comment string. */

                    if (recBuff[0] == '#') {
                        try {               // wait before sending "send temp"
                            sleep(5000);
                        } catch (InterruptedException e) {
                            Log.d(TAG, "HELLO_SENT: sleep interrupted");
                        }

                        state = IDLE;

                        break;
                    }

                    tokens = recString.split("\\s+");    // tokenize received string
                    if (helloSequence != null) {
                        tokens2 = helloSequence.split("\\s+");     // tokenize transmitted "ack <seq>"
                    }

                    /* Check if "ack <seq> received. */

                    if (tokens[0].equals("ack:")) {
                        if (helloSequence == null) Log.d(TAG, "helloSequence null");
                        if (tokens2 == null) Log.d(TAG, "tokens2 null");
                        if ((helloSequence != null) & (tokens2 != null)) {
                            if (tokens2[1].equals(tokens[1])) {
                                Log.d(TAG, "ack with correct sequence received");
                                state = LINK_ESTABLISHED;
                            } else {
                                Log.d(TAG, "ack with incorrect sequence received");
                                state = IDLE;
                            }
                        }
                    } else {
                        Log.d(TAG, "Unexpected response to \"hello <seq>\"");
                        state = IDLE;
                    }

                    try {               // wait before sending "send temp"
                        sleep(5000);
                    } catch (InterruptedException e) {
                        Log.d(TAG, "HELLO_SENT: sleep interrupted");
                    }

                    break;

                /* state LINK_ESTABLISHED: send "send temp" */

                case LINK_ESTABLISHED:

                    try {
                        byte[] data = "send: temp\n".getBytes("UTF-8");
                        mSpiDevice.write(data, data.length);
                        Log.d(TAG, "Tx: " + "send: temp");
                    } catch (IOException e) {
                        Log.d(TAG, "LINK_ESTABLISHED: write of \"send temp\" failed.");
                    }

                    try {
                        sleep(150);
                    } catch (InterruptedException e) {
                        Log.d(TAG, "LINK_ESTABLISHED: sleep interrupted.");
                    }

                    state = SEND_SENT; // update state

                    break;

                /* SEND_SENT: try to receive "temp <value>" */

                case SEND_SENT:
                    try {
                        recBuff[0] = '\n';
                        mSpiDevice.read(recBuff, REC_BUFF_LENG);
                    } catch (IOException e) {
                        Log.d(TAG, "SEND_SENT: read from slave failed.");
                        try {
                            sleep(5000);
                        } catch (InterruptedException e1) {
                            Log.d(TAG, "SEND_SENT: sleep interrupted");
                        }
                        state = IDLE;
                        break;
                    }

                    /* Check to see if received message is an ack. */

                    recString = "yyy";

                    recBuff[0] &= 0x7f;    // ****** HACK: ****** clear upper bit of first received byte

                    found = false;

                    /* Terminate received message at first '\n' char. */

                    for (int i = 0; i < recBuff.length; i++) {
                        if (recBuff[i] == '\n') {
                            recBuff[i] = 0;
                            found = true;
                            try {
                                recString = new String(recBuff, 0, i, "UTF-8");
                            } catch (UnsupportedEncodingException e) {
                                Log.d(TAG, "SEND_SENT: UnsupportedEncodingException.");
                            }
                            break;      // is this right???
                        }
                    }

                    if (!found) {
                        Log.d(TAG, String.format("not found %02x, %02x, %02x", recBuff[0], recBuff[1], recBuff[2]));
                        try {
                            recString = new String(recBuff, 0, recBuff.length, "UTF-8");
                        } catch (UnsupportedEncodingException e) {
                            Log.d(TAG, "HELLO_SENT: UnsupportedEncodingException.");
                        }
                    }


                    Log.d(TAG, "Rx: " + recString);

                    /* Check for a comment string. */

                    if (recBuff[0] == '#') {
                        try {               // wait before sending "send temp"
                            sleep(5000);
                        } catch (InterruptedException e) {
                            Log.d(TAG, "HELLO_SENT: sleep interrupted");
                        }

                        state = IDLE;

                        break;
                    }

                    tokens = recString.split("\\s+");    // tokenize received string

                    /* Check if "ack temp" received. */

                    if (tokens[0].equals("temp:")) {
                        Log.d(TAG, "temp: received " + tokens[1]);
                        activity.setI2CTemp(Double.valueOf(tokens[1]));
                        state = LINK_ESTABLISHED;
                    } else {
                        Log.d(TAG, "Unexpected response to \"send temp\"");
                        state = IDLE;
                    }

                    try {               // wait before receiving temp values
                        sleep(5000);
                    } catch (InterruptedException e) {
                        Log.d(TAG, "HELLO_SENT: sleep interrupted");
                    }

                    break;

                /* state LINK_ACTIVE_ try to receive "temp: <value>" */

                case LINK_ACTIVE:
                    try {
                        recBuff[0] = '\n';
                        mSpiDevice.read(recBuff, REC_BUFF_LENG);
                    } catch (IOException e) {
                        Log.d(TAG, "LINK_ACTIVE: read temp from slave failed.");
                        try {
                            sleep(5000);
                        } catch (InterruptedException e1) {
                            Log.d(TAG, "LINK_ACTIVE: sleep interrupted");
                        }
                        state = IDLE;
                        break;
                    }

                    /* Check to see if received message is an ack. */

                    recString = "zzz";

                    recBuff[0] &= 0x7f;    // ****** HACK: ****** clear upper bit of first received byte

                    /* Terminate received message at first '\n' char. */

                    for (int i = 0; i < recBuff.length; i++) {
                        if (recBuff[i] == '\n') {
                            recBuff[i] = 0;
                            Log.d(TAG, "recBuff i: " + i);
                            try {
                                recString = new String(recBuff, 0, i, "UTF-8");
                            } catch (UnsupportedEncodingException e) {
                                Log.d(TAG, "LINK_ACTIVE: UnsupportedEncodingException.");
                            }
                            break;      // is this right???
                        }
                    }

                    Log.d(TAG, "Rx: " + recString);

                    tokens = recString.split("\\s+");
                    for (int i = 0; i < tokens.length; i++) {
                        Log.d(TAG, "token: \"" + tokens[i] + "\"");
                    }


                    try {
                        sleep(5000);
                    } catch (InterruptedException e) {
                        Log.d(TAG, "LINK_ACTIVE: sleep interrupted");
                    }

                    state = LINK_ACTIVE;
            }
        }
    }
}
