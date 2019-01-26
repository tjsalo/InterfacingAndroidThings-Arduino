/** MainActivity - Demonstrate integrating Android Things with an Arduino
 * board using async, I2C, and SPI interfaces.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */


package com.salo.android.arduinointegration;

import android.app.Activity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;

import java.io.IOException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.time.Duration;
import java.time.Instant;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import android.util.Log;
import android.widget.TextView;

import com.google.android.things.contrib.driver.ssd1306.Ssd1306;

import fi.iki.elonen.NanoHTTPD;



/* MainActivity
 */

public class MainActivity extends Activity implements AndroidWebServer.WebserverListener {

    private static final String TAG = MainActivity.class.getSimpleName();

    /* Async, I2C and SPI devices. */

    private static final String UART_DEVICE_1_NAME = "UART0";    // UART on GPIO connector
    private static final String UART_DEVICE_2_NAME = "USB1-1.4:1.0";    // USB TTL serial dongle
    private static final String I2C_BUS = "BUS NAME";
    private static final String I2C_DEVICE_NAME = "I2C1";    // I2C device name
    private static final int I2C_ADDRESS= 0x77;    // I2C slave address
    private static final String SPI_DEVICE_NAME = "SPI0.0";

    private Ssd1306 mScreen;

    private TextView mTextView;
    private String mString;

    private AndroidWebServer aws;

    /* Temperature sensor readings from Arduino. */

    private double tempAsync = -1.0;    // temp from async interface
    private Instant timeAsync = null;   // time of last async update

    private double tempI2C = -1.0;      // temp from I2C interface
    private Instant timeI2C = null;     // time of last I2C update

    private double tempSPI = -1.0;           // temp from SPI interface
    private Instant timeSPI = null;     // time of last SPI update

    /* Web statistics. */

    private int pageRequests = 0;       // number of HTTP GET requests



    /* onCreate - Initialize Activity.
     */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        setupOledDisplay();
        setContentView(R.layout.activity_main);

        Log.d(TAG, "onCreate()");

        getActionBar().setTitle("Arduino / Android Things Integration");

        mTextView = (TextView) findViewById(R.id.main_textview);
        mTextView.setMovementMethod(new ScrollingMovementMethod());
        mTextView.setText("");

        addTextMessage("Arduino / Android Things Integration Demo\n");

        /* List network interfaces. */

        try {
            String string;
            Enumeration<NetworkInterface> nets = NetworkInterface.getNetworkInterfaces();
            for (NetworkInterface netint : Collections.list(nets)) {
                string = "Interface: " + netint.getDisplayName();
                Enumeration<InetAddress> addresses = netint.getInetAddresses();
                for (InetAddress address : Collections.list(addresses)) {
                    byte bytes[] = address.getAddress();
                    if (bytes.length == 4) {
                        string = string + " " + address.toString().substring(1);
                    }
                }
                Log.d(TAG, string);
                addTextMessage(string + "\n");
            }
        } catch (SocketException e) {
            e.printStackTrace();
        }

        /* Start web server.
         * FIXME: should this be off the UI thread?
         */

        aws = new AndroidWebServer(8080, this);
        aws.setListener(this);

        addTextMessage("Web Server Running\n");

        /* Fire up threads to communicate over interfaces. */

        AsyncHandlerThread asyncHandlerThread1 = new AsyncHandlerThread(this,UART_DEVICE_1_NAME);
        asyncHandlerThread1.start();

        AsyncHandlerThread asyncHandlerThread2 = new AsyncHandlerThread(this, UART_DEVICE_2_NAME);
        asyncHandlerThread2.start();

        I2cHandlerThread i2cThread = new I2cHandlerThread(this, I2C_DEVICE_NAME, I2C_ADDRESS);
        i2cThread.start();

        SpiHandlerThread spiHandlerThread = new SpiHandlerThread(this, SPI_DEVICE_NAME);
//  ****** debug ******      spiHandlerThread.start();
    }



    /* onDestroy - clean up on Activity termination.
     */

    @Override
    protected void onDestroy() {
        super.onDestroy();
//        destroyOledDisplay();
    }



    /* setupOledDisplay - this doesn't work anyway...
     */

    private void setupOledDisplay() {
        try {
            mScreen = new Ssd1306(I2C_BUS);
        } catch (IOException e) {
            Log.e(TAG, "Error while opening screen", e);
        }
        Log.d(TAG, "OLED screen activity created");
    }



    /* destroyOledDisplay - make this work someday.
     */

    private void destroyOledDisplay() {
        if (mScreen != null) {
            try {
                mScreen.close();
            } catch (IOException e) {
                Log.e(TAG, "Error closing SSD1306", e);
            } finally {
                mScreen = null;
            }
        }
    }



    /* handleHttpGet - generate response to HTTP GET request.
     */

    @Override
    public String handleHttpGet(NanoHTTPD.IHTTPSession session){

        String html;                    // HTML returned to browser

        pageRequests++;                 // increment page count

        Map parms = session.getParms();

        Log.d(TAG, "parms.size: " + parms.size());
        Log.d(TAG, "parms.keys: " + parms.keySet().toString());

        Map headers = session.getHeaders();

        Log.d(TAG, "headers.size: " + headers.size());
        Log.d(TAG, "headers.keys: " + headers.keySet().toString());

        Log.d(TAG, "Request from host: " + headers.get("remote-addr") + " for: " +
                session.getUri());

        mString = "Request from host: " + headers.get("remote-addr") + " for: " +
                session.getUri() + "\n";
        runOnUiThread(new Runnable() {

            @Override
            public void run() {
                mTextView = (TextView) findViewById(R.id.main_textview);
                mTextView.setText(mTextView.getText() + mString);
                mTextView.invalidate();
                Log.d(TAG, "runOnUiThread: " + mString);
                Log.d(TAG, "runOnUiThread: " + mTextView.getText());

            }
        });

        Set set = headers.keySet();
        Iterator<String> iterator = set.iterator();

        while (iterator.hasNext()) {
            String next = iterator.next();
            Log.d(TAG, " Key: " + next + " Value: " + headers.get(next));
        }

        Log.d(TAG, "URI: " +session.getUri());

        Log.d(TAG, "Query Parameters: " + session.getQueryParameterString());

        html = "<html>\n" +
                "<head>\n" +
                "<title>Arduino / Android Things Integration Demo</title>\n" +
                "</head>\n" +
                "<body>\n" +
                "<h1 align=\"center\">Arduino / Android Things Integration Demo</h1>\n" +
                "<h2 align=\"center\">Timothy J. Salo</h2>\n" +
                "<h3 align=\"center\">Salo IT Solutions, Inc.</h3>\n";

        double time;                    // time since last update
        Instant now = Instant.now();    // current time

        /* Build up async temp display. */

        if (timeAsync != null) {
            time = Duration.between(timeAsync, now).toMillis() / 1000.0;
        } else {
            time = -1.0;
        }

        html += String.format("<p style=\"font-size:150%%;\">Temperature from async interface: %6.2f, (%6.3f seconds ago)</p>\n",
                tempAsync, time);

        /* Build up I2C temp display. */

        if (timeI2C != null) {
            time = Duration.between(timeI2C, now).toMillis() / 1000.0;
        } else {
            time = -1.0;
        }

        html += String.format("<p style=\"font-size:150%%;\">Temperature from I2C interface:   %6.2f, (%6.3f seconds ago)</p>\n",
                tempI2C, time);

        /* Build up SPI temp display. */

        if (timeSPI != null) {
            time = Duration.between(timeSPI, now).toMillis() / 1000.0;
        } else {
            time = -1.0;
        }

        html += String.format("<p style=\"font-size:150%%;\">Temperature from SPI interface:   %6.2f, (%6.3f seconds ago)</p>\n",
                tempSPI, time);

        /* Display number of page loads. */

        html += String.format("<p>Page loads: %d</p>\n", pageRequests);

        html += "</body>\n" +
                "</html>\n";

        return html;
    }


    private void addTextMessage(String string) {
        Log.d(TAG, "addTextmessage: " + string);
//        mTextView = (TextView) findViewById(R.id.main_textview);
        mTextView.setText(mTextView.getText() + string);
        Log.d(TAG, "addTextMessage: " + mTextView.getText());
    }



    /* setAsyncTemp - set temp from async interface.
     */

    public void setAsyncTemp(double temp) {
        tempAsync = temp;
        timeAsync = Instant.now();
    }



    /* setI2CTemp - set temp from I2C interface.
     */

    public void setI2CTemp(double temp) {
        tempI2C = temp;
        timeI2C = Instant.now();
    }



    /* setSPITemp - set temp from SPI interface.
     */

    public void setSPITemp(double temp) {
        tempSPI = temp;
        timeSPI = Instant.now();
    }
}

