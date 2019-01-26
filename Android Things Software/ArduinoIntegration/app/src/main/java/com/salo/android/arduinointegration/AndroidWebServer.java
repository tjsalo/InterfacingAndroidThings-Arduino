package com.salo.android.arduinointegration;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import fi.iki.elonen.NanoHTTPD;


public class AndroidWebServer extends NanoHTTPD {

    private static final String TAG = "HTTPServer";
    private Context context;
    private WebserverListener listener;


    /* AndroidWebServer() - constructor.
     */

    public AndroidWebServer(int port, Context context) {
        super(port);
        this.context = context;
        try {
            Log.d("TAG", "Starting web server..");
            start();
        }
        catch(IOException ioe) {
            Log.e(TAG, "Unable to start the server");
            ioe.printStackTrace();
        }
    }


    /* setListener - set callback.
     */

    public void setListener(WebserverListener listener) {
        this.listener = listener;
    }


    /* serve - respond to requests from NanoHTTPD.
     */

    @Override
    public Response serve(IHTTPSession session) {

        /* Process requests for favicon.ico locally. */

        String uri = session.getUri();

        if ("/favicon.ico".equalsIgnoreCase(uri)) {
            Log.d(TAG, "Found favicon.ico");
            AssetManager assetManager = context.getAssets();
            try {
                InputStream inputStream = assetManager.open("favicon.ico");
                return newChunkedResponse(Response.Status.OK, "image/x-icon", inputStream);
            } catch (IOException e) {
                Log.d(TAG, e.toString());
            }
        }

        /* Handle everything except favicon.ico. */

        return newFixedLengthResponse(listener.handleHttpGet(session));
    }


    /* stopServer() - stop web server.
     */

    public void stopServer() {
        this.stop();
    }


    /* WebserverListener - callback to generate content.
     */

    public interface WebserverListener {
        public String handleHttpGet(IHTTPSession session);
    }
}
