package no.gknudsen.trafikklys;

import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.sql.Timestamp;
import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

    ScanTask networktask;
    TextView notif, ipfield, tempoIndicator;
    View cancelButton, setupButton, tempoMinusButton, tempoPlusButton, changeProgram, resetProgram, calibrateButton, testUDPButton;
    SeekBar tempoSlider;
    //ArrayList<Client> clients = new ArrayList<>();
    ArrayList<Message> messages = new ArrayList<>();

    public int calibrate_index = -1;
    public static final byte[] CALIBRATE = {
            (byte) 0b00000001,
            (byte) 0b00000010,
            (byte) 0b00000100
    };

    public static final byte[][] PROGRAMS = {
        {
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000100,
                (byte) 0b00000001,
                (byte) 0b00000001,
                (byte) 0b00000001,
                (byte) 0b00000001,
                (byte) 0b00000001,
                (byte) 0b00000001,
                (byte) 0b00000001,
                (byte) 0b00000001
        },
        {
                (byte) 0b00000100,
                (byte) 0b00000010,
                (byte) 0b00000001,
                (byte) 0b00000010
        },
        {
                (byte) 0b00000101,
                (byte) 0b00000010,
                (byte) 0b00000101
        }
    };


    int tempo = 100;
    int program = 0;


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
        tempoMinusButton = findViewById(R.id.tempoMinusButton);
        tempoPlusButton = findViewById(R.id.tempoPlusButton);
        changeProgram = findViewById(R.id.changeProgram);
        resetProgram = findViewById(R.id.resetProgram);
        tempoIndicator = findViewById(R.id.tempoIndicator);
        calibrateButton = findViewById(R.id.calibrateButton);
        setupButton = findViewById(R.id.setupButton);
        testUDPButton = findViewById(R.id.testUDPButton);
        tempoSlider = findViewById(R.id.tempoSlider);


        // perform seek bar change listener event used for getting the progress value
        tempoSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tempo = progress;
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                seekBar.setProgress(tempo);
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                submitTempo(tempo);
            }
        });

        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.i("MainActivity", "Cancel scan");
                networktask.cancel(true);
            }
        });

        changeProgram.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                calibrate_index = -1;

                program++;
                if (program == PROGRAMS.length) program = 0;
                int len = PROGRAMS[program].length;
                Log.v("JOAKIM", "Send program #" + program + " (" + len + " bytes)");
                byte[] payload = new byte[len + 2];
                payload[0] = 3;
                payload[1] = (byte) len;
                for (int i = 0; i < len; i++) {
                    payload[i + 2] = PROGRAMS[program][i];
                }
                if (Utility.clients.size() < 1) {
                    Log.e("MainActivity", "No clients");
                } else {
                    broadcast(payload);
                    // TODO: Send different parts of program to different clients...
                    //clients.get(0).transmit(payload);
                }
            }
        });

        resetProgram.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.v("JOAKIM", "Send reset");
                byte[] payload = {4, 1, 0};  // TODO: Støtte payload null
                if (Utility.clients.size() < 1) {
                    Log.e("MainActivity", "No clients");
                } else {
                    broadcast(payload);
                    //clients.get(0).transmit(payload);
                }
            }
        });

        testUDPButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.v("JOAKIM", "Trigger sending UDP broadcast");  // only ONE client (master) should respond to this message
                byte[] payload = {5, 1, 0};  // TODO: Støtte payload null
                if (Utility.clients.size() < 1) {
                    Log.e("MainActivity", "No clients");
                } else {
                    broadcast(payload);
                    //clients.get(0).transmit(payload);
                }
            }
        });


        setupButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });

        calibrateButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (calibrate_index < 0) {
                    Log.i("MainActivity", "Start calibration");
                    calibrate_index = 0;
                } else calibrate_index++;
                if (calibrate_index > 2) calibrate_index = 0;
                

                // send program + reset
                byte[] payload = new byte[3];
                payload[0] = 3;
                payload[1] = 1;
                payload[2] = CALIBRATE[calibrate_index];
                if (Utility.clients.size() < 1) {
                    Log.e("MainActivity", "No clients");
                } else {
                    broadcast(payload);
                }

                byte[] resetMessage = {4, 1, 0};
                if (Utility.clients.size() < 1) {
                    Log.e("MainActivity", "No clients");
                } else {
                    broadcast(resetMessage);
                }
            }
        });

        tempoMinusButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                tempo -= 1;
                submitTempo(tempo);
            }
        });

        tempoPlusButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                tempo += 1;
                submitTempo(tempo);
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
        networktask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, ip);
    }

    private void submitTempo(int tempo) {

        if (tempo < 0) tempo = 1;
        if (tempo > 255) tempo = 255;

        Log.v("JOAKIM", "Send +tempo: " + tempo);
        tempoIndicator.setText("tempo: " + tempo);
        byte[] payload = {1, 1, (byte) tempo};
        if (Utility.clients.size() < 1) {
            Log.e("MainActivity", "No clients");
        } else {
            broadcast(payload);
            //clients.get(0).transmit(payload);
        }
    }

    private void updateAdapter() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Utility.getAdapter().notifyDataSetChanged();
            }
        });
    }

    public class ScanTask extends AsyncTask<String, Integer, Boolean> {

        @Override
        protected void onPreExecute() {
            Log.i("AsyncTask", "onPreExecute");
        }

        @Override
        protected Boolean doInBackground(String... params) { //This runs on a different thread
            boolean result = false;
            String ip = params[0];
            ServerSocket serverSocket;

            try {

                InetAddress addr = InetAddress.getByName(ip);  //"192.168.43.86");
                Log.i("Server", "Server starting at port number: " + PORTNUMBER);
                serverSocket = new ServerSocket(PORTNUMBER, 5, addr);
                notif.setText("Server starting at " + ip + ":" + PORTNUMBER);

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
                    Log.i("Server", "Client has connected.");


                    Client newClient = new Client(clientID++, socket,
                            new Client.OnMessageReceived() {
                                @Override
                                public void messageReceived(Message message) {
                                    messages.add(message);
                                }

                                @Override
                                public void onInit(long clientID, int ID, Timestamp created) {
                                    // cleanup
/*
                                    for (Client c : new ArrayList<>(Utility.clients)) {
                                        if (c.clientID == clientID && c.ID != ID && c.created.before(created)) {
                                            Log.v("MainActivity", "Replacing dead client " + c.ID + " (" + c.clientID + ")");
                                            Utility.clients.remove(c);
                                            updateAdapter();
                                        }
                                    }
                                    publishProgress(Utility.clients.size());
                                    */
                                }
                            });

                    Utility.clients.add(newClient);
                    updateAdapter();
                    runOnUiThread(new Runnable() {

                        @Override
                        public void run() {

                            // Stuff that updates the UI

                        }
                    });

                    publishProgress(Utility.clients.size());
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

    private void broadcast(byte[] payload) {
        for (Client client : Utility.clients) {
            client.transmit(payload);
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
