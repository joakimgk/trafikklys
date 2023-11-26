package com.example.trafikklys2;

import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.sql.Timestamp;

public class Client {

    public interface OnMessageReceived {
        public void messageReceived(Message message);
        public TrafficLight onInit(long clientID, int ID, Timestamp created);
    }

    Socket nsocket;
    private OnMessageReceived mMessageListener;
    Timestamp created;
    int ID;
    long clientID;
    byte[] buffer;
    TrafficLight mTrafficLight;

    boolean active = false;

    public Client (long cid) {
        clientID = cid;
    }

    public Client (int ID, Socket socket, OnMessageReceived listener) {
        this.ID = ID;
        nsocket = socket;
        mMessageListener = listener;
        //rotation = ID % 2 == 0 ? 90 : 0;
        created = new Timestamp(System.currentTimeMillis());

        // new ReadTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        if (socket != null) startReceiver();
    }

    public void startReceiver() {
        new ReadTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void setTrafficLight(TrafficLight trafficLight) {
        mTrafficLight = trafficLight;
    }

    boolean isBusy = false;

    public void transmit(byte[] payload) {
        new SendTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, payload);
    }

    public class ReadTask extends AsyncTask<byte[], byte[], Boolean> {

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            Log.i("AsyncTask", "STARTING READER on client " + ID + "...");
        }

        @Override
        protected Boolean doInBackground(byte[]... bytes) {
            while (nsocket != null) {
                while (nsocket == null || !nsocket.isConnected()) {
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

                try {
                    //
                    BufferedInputStream input = new BufferedInputStream(nsocket.getInputStream());
                    int i = 0;
                    byte[] tempdata = new byte[4096];


                    while (input.available() > 0) {
                        tempdata[i] = (byte) input.read();
                        Log.v("Reading", "byte #" + i + ": " + (char) tempdata[i]);
                        i++;
                    }
                    if (i == 0) continue;
                    tempdata[i] = '\0';

                /*
                                int bytesRead = 0;
                while ((bytesRead = input.read(tempdata)) != -1) {
                    for (int j = 0; j < bytesRead; j++) {
                        byte u = (byte) input.read();
                        tempdata[i] = u;
                        Log.v("Reading", "byte #" + j + " of " + bytesRead + ": " + (char)u);
                        i++;
                    }
                    if (input.available() <= 0) break;
                }
                 */

                    Log.v("Reading", "done: " + i + " bytes");
                    buffer = new byte[i + 1];
                    System.arraycopy(tempdata, 0, buffer, 1, i);
                    buffer[0] = '?';

                    if (buffer != null && mMessageListener != null) {
                        mMessageListener.messageReceived(new Message(ID, buffer));

                        Uri uri = Uri.parse(new String(buffer));
                        String p_clientID = uri.getQueryParameter("clientID");
                        if (p_clientID != null) {
                            Long l = Long.parseLong(p_clientID);
                            clientID = l.longValue();
                            mTrafficLight = mMessageListener.onInit(clientID, ID, created);
                            active = true;
                            Log.v("Reading", "clientID = " + clientID);
                        }
                    }
                    buffer = null;

                } catch (IOException e) {
                    Log.i("AsyncTask", "error");
                    e.printStackTrace();
                }

            /*

            try {
                nis = nsocket.getInputStream();

                buffer = new byte[4096];
                int read = nis.read(buffer, 0, 4096); //This is blocking
                while(read != -1){
                    hasData = true;
                    byte[] tempdata = new byte[read];
                    System.arraycopy(buffer, 0, tempdata, 0, read);
                    publishProgress(tempdata);
                    Log.i("AsyncTask", "doInBackground: Got some data... " + tempdata.length + " bytes");
                    read = nis.read(buffer, 0, 4096); //This is blocking
                }
                if (buffer != null && mMessageListener != null) {
                    mMessageListener.messageReceived(new Message(ID, buffer));
                }
            } catch (IOException e) {
                Log.i("AsyncTask", "error");
                e.printStackTrace();
            }
            */
            }
            return null;
        }
    }

    public class SendTask extends AsyncTask<byte[], byte[], Boolean> {

        @Override
        protected void onPreExecute() {
            Log.i("SendTask", "onPreExecute");
        }

        @Override
        protected Boolean doInBackground(byte[]... params) { //This runs on a different thread
            Log.i("SendTask", "transmit command 0x0" + params[0][0] + " (" + params[0].length + " bytes)");

            while (nsocket == null || !nsocket.isConnected()) {  // || isBusy) {
                try {
                    Thread.sleep(500);
                    Log.i("SendTask", "Client (send thread) busy...");
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }

            boolean result = false;
            isBusy = true;


            try {

                if (nsocket.isConnected()) {
                    //PrintWriter out = new PrintWriter(new BufferedWriter(
                    //        new OutputStreamWriter(nsocket.getOutputStream())), true);
                    DataOutputStream dOut = new DataOutputStream(nsocket.getOutputStream());
                    Log.i("SENDING", "" + params[0][0] + params[0][1] + params[0][2]);
                    dOut.write(params[0], 0, params[0].length);
                    result = true;
                } else {
                    Log.e("SendTask", "nsocket not connected");
                }

            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                isBusy = false;
            }
            return result;
        }

        @Override
        protected void onPostExecute(Boolean result) {
            if (!result) {
                Log.i("AsyncTask", "onPostExecute: Completed with an Error.");
                Log.e("JOAKIM", "There was a connection error.");
            } else {
                Log.i("AsyncTask", "onPostExecute: Completed.");
            }
        }
    }
}

