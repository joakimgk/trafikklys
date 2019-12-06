package no.gknudsen.trafikklys;

import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.Socket;

public class Client {

    Socket nsocket;
    int ID;

    public Client (int ID, Socket socket) {
        this.ID = ID;
        nsocket = socket;
    }

    public void transmit(byte[] payload) {
        new SendTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, payload);
    }

    public class SendTask extends AsyncTask<byte[], byte[], Boolean> {

        @Override
        protected void onPreExecute() {
            Log.i("SendTask", "onPreExecute");
        }

        @Override
        protected Boolean doInBackground(byte[]... params) { //This runs on a different thread
            boolean result = false;

            Log.i("SendTask", "transmit " + params[0].length + " bytes");
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

