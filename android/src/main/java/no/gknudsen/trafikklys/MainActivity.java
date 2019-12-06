package no.gknudsen.trafikklys;

import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

    ScanTask networktask;
    TextView notif, ipfield;
    View cancelButton, sendButton;
    ArrayList<Client> clients = new ArrayList<>();

    public static final int PORTNUMBER = 10000;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        notif = findViewById(R.id.text);
        ipfield = findViewById(R.id.text2);
        cancelButton = findViewById(R.id.cancelScanButton);
        sendButton = findViewById(R.id.sendButton);

        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.i("MainActivity", "Cancel scan");
                networktask.cancel(true);
            }
        });

        sendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.i("MainActivity", "Send something!");
                byte[] payload = {1, 1, 30};
                if (clients.size() < 1) {
                    Log.e("MainActivity", "No clients");
                } else {
                    clients.get(0).transmit(payload);
                }
            }
        });

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        WifiManager wm = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        String ip = Formatter.formatIpAddress(wm.getConnectionInfo().getIpAddress());
        ipfield.setText("ip = " + ip);

        networktask = new ScanTask(); //Create initial instance so SendDataToNetwork doesn't throw an error.
        networktask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public class ScanTask extends AsyncTask<Void, Integer, Boolean> {

        @Override
        protected void onPreExecute() {
            Log.i("AsyncTask", "onPreExecute");
        }

        @Override
        protected Boolean doInBackground(Void... params) { //This runs on a different thread
            boolean result = false;
            ServerSocket serverSocket;
            try {

                InetAddress addr = InetAddress.getByName("192.168.43.86");
                Log.i("Server", "Server starting at port number: " + PORTNUMBER);
                serverSocket = new ServerSocket(PORTNUMBER, 5, addr);
                notif.setText("Server starting at port number: " + PORTNUMBER);

/*
That would need explicit support from the emulator (some sort of driver to automatically bind ports on the host when something on the emulated device binds to a port on the loopback device), which I'm pretty sure it doesn't have.

You can use adb to set up port forwardings from a port on the host to a port on the device. I don't know if the emulator has a more direct way of accomplishing the same thing.
Yes, sounds like it does, via the redir command of https://developer.android.com/studio/run/emulator-console

That said, "redir add tcp:10001:10001" on the emulator console should make it possible to access the ServerSocket on port 10001 in the emulator by connecting to port 10001 of your laptop's address.

 */
                int clientID = 1;
                while(!isCancelled()) {
                    // https://stackoverflow.com/questions/41939101/android-client-and-server-sockets
                    // Client connecting.
                    Log.i("Server", "Waiting for clients to connect ...");

                    Socket socket = serverSocket.accept();
                    Log.i("Server", "Client one has connected.");

                    clients.add(new Client(clientID++, socket));
                    publishProgress(clients.size());
                }
                notif.setText("DONE");

            } catch (IOException e) {
                e.printStackTrace();
            }
            return result;
        }

        @Override
        protected void onProgressUpdate(Integer... progress) {
            notif.setText(progress[0] + " clients connected ...");
        }
        @Override
        protected void onPostExecute(Boolean result) {
            if (result) {
                Log.i("AsyncTask", "onPostExecute: Completed with an Error.");
                Log.e("JOAKIM", "There was a connection error.");
            } else {
                Log.i("AsyncTask", "onPostExecute: Completed.");
            }
        }
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
