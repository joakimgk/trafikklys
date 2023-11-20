package com.example.trafikklys2;

import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Iterator;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    ScanTask networktask;
    static TextView notif;
    TextView ipfield;
    TextView tempoIndicator;
    View cancelButton, setupButton, tempoMinusButton, tempoPlusButton, changeProgram, resetProgram, calibrateButton, testUDPButton;
    SeekBar tempoSlider;
    //ArrayList<Client> clients = new ArrayList<>();
    ArrayList<Message> messages = new ArrayList<>();
    boolean isRunning = true;

    public int calibrate_index = -1;

    public static byte NON = (byte) 0b00000000;
    public static byte RED = (byte) 0b00000001;
    public static byte YLW = (byte) 0b00000010;
    public static byte GRN = (byte) 0b00000100;

    public static byte ALL = (byte) 0b00000111;

    public static final byte[] IDENTIFY  = {
            ALL,
            NON
    };

    public static final byte[] CALIBRATE = {
            RED, YLW, GRN
    };

    public static final byte[] PAUSE = {
            (byte) 0x00,
            (byte) 0x00,
            (byte) 0x00
    };

    public static final byte[][] PROGRAMS = {
            {
                    GRN, GRN, GRN, GRN, GRN, GRN, GRN, GRN,
                    RED, RED, RED, RED, RED, RED, RED, RED
            },
            {
                    GRN, YLW, RED, YLW
            },
            {
                    (byte) (RED | GRN), YLW, (byte) (RED | GRN)
            }
    };

    int tempo = 100;
    int program = 0;


    public static final int PORTNUMBER = 10000;

    static TrafficLightContainer mContainer;

    private static Iterator<Client> mClientIterator;

    static int mNumConfigured = 0;
    private PingTask pingTask;

    static byte[] resetMessage = {4, 1, 0};  // reset (restart)

    public static void mapClientTrafficLight() {
        if (mClientIterator.hasNext()) {
            if (Utility._mapClient != null) {
                //byte[] configDoneMessage = {0x0A, 1, 0};  // config done
                //Utility._mapClient.transmit(configDoneMessage);
                // stop blinking (static program to indicate done)
                Utility._mapClient.transmit(
                        Animate.concat(programPayload(new byte[]{ALL}), resetMessage)
                );
            }

            Utility._mapClient = mClientIterator.next();
            /*broadcast(PAUSE);
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
             */
            mNumConfigured++;
            notif.setText(mNumConfigured + " / " + Utility.clients.size() + " configured");
            Utility._mapClient.transmit(
                    Animate.concat(programPayload(IDENTIFY), resetMessage)
            );

        } else {

            mContainer.mAssigning = false;
            startShow();
            notif.setText("Configure done!");

            /*
            broadcast(programPayload(CALIBRATE));
            try {
                Thread.sleep(700);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            byte[] payload = {4, 1, 0};  // reset (restart)
            broadcast(payload);
             */
        }
    }

    private static void startShow() {
        Utility.updateGroups();
        //transmitProgram(Animate.Trail());
        //transmitProgram(Animate.Wave());
        //transmitProgram(Animate.Sink());
        //transmitProgram(Animate.Pendulum());

        ArrayList<Program> p = Animate.Trail();
        transmitProgram(Animate.Trail());
    }

    private static void transmitProgram(ArrayList<Program> program) {
        for (Program p : program) {
            p.mClient.transmit(
                    Animate.concat(programPayload(p.mProgram), resetMessage)
                    );
        }
       
    }


    private static byte[] programPayload(byte[] program) {
        int len = program.length;
        byte[] payload = new byte[len + 2];
        payload[0] = 3;
        payload[1] = (byte) len;
        for (int i = 0; i < len; i++) {
            payload[i + 2] = program[i];
        }
        return payload;
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
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
                tempoIndicator.setText("tempo: " + tempo);

                // repeat large tempo changes 3 times, in case of packet loss
                for (int i = 0; i < 3; i++) {
                    submitTempo(tempo);

                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
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
                //if (Utility.clients.size() < 1) {
                //    Log.e("MainActivity", "No clients");
                //} else {
                    broadcast(programPayload(PROGRAMS[program]));
                    // TODO: Send different parts of program to different clients...
                    //clients.get(0).transmit(payload);
                //}
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
                mNumConfigured = 0;

                if (mContainer.mAssigning) {
                    mContainer.mAssigning = false;
                    Log.v("JOAKIM", "Aborting setup...");
                    return;
                }
                notif.setText(Utility.clients.size() + " clients connected: Please configure");

                byte[] nullProgram = { 3, 1, 0 };
                byte[] resetMsg = {4, 1, 0};
                broadcast(Animate.concat(nullProgram, resetMsg));
                mContainer.mAssigning = true;
                mClientIterator = Utility.clients.iterator();
                mapClientTrafficLight();
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
                tempoIndicator.setText("tempo: " + tempo);
                submitTempo(tempo);
            }
        });

        tempoPlusButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                tempo += 1;
                tempoIndicator.setText("tempo: " + tempo);
                submitTempo(tempo);
            }
        });

        WifiManager wm = (WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE);
        String ip = Formatter.formatIpAddress(wm.getConnectionInfo().getIpAddress());
        ipfield.setText("ip = " + ip);

        mContainer = findViewById(R.id.traffic_light_container);
        networktask = new ScanTask(); //Create initial instance so SendDataToNetwork doesn't throw an error.
        networktask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, ip);
        
        pingTask = new PingTask();
        pingTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

        if (false) {
            // test:
            int numClients = 3;
            int[] orientations = { 0, 90, 0};
            Utility.clients = new ArrayList<>(numClients);
            for (int i = 0; i < numClients; i++) {
                Client c = new Client((long)i);
                Utility.clients.add(c);
                c.mTrafficLight = new TrafficLight(this, i, orientations[i]);
                mContainer.addCell();
            }
            Utility.updateGroups();

            startShow();
        }
    }

    private void submitTempo(int tmpo) {
        if (tmpo < 0) tmpo = 1;
        if (tmpo > 255) tmpo = 255;
        //Log.v("JOAKIM", "Send +tempo: " + tmpo);
        byte[] payload = {1, 1, (byte) tmpo};
        broadcast(payload);

    }

    public class PingTask extends AsyncTask<Void, Void, Boolean> {
        
        @Override
        protected Boolean doInBackground(Void... voids) {
            while(isRunning && Utility.clients.size() < 2) {
                submitTempo(tempo);
                try {
                    Thread.sleep(5*1000);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }
            return null;
        }
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
                                public TrafficLight onInit(long clientID, int ID, Timestamp created) {
                                    // cleanup
                                    for (Client c : new ArrayList<>(Utility.clients)) {
                                        if (c.clientID == clientID && c.ID != ID && c.created.before(created)) {
                                            Log.v("MainActivity", "Replacing dead client " + c.ID + " (" + c.clientID + ")");
                                            Utility.clients.remove(c);
                                            publishProgress(Utility.clients.size());
                                            return c.mTrafficLight;
                                        }
                                    }
                                    // new client (not seen before)
                                    publishProgress(Utility.clients.size());

                                    runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            // Stuff that updates the UI
                                            mContainer.addCell();
                                        }
                                    });

                                    return null;
                                }
                            });

                    Utility.clients.add(newClient);
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

    private static void broadcast(byte[] payload) {
        new BroadcastTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, payload);
    }

    public static class BroadcastTask extends AsyncTask<byte[], byte[], Boolean> {

        @Override
        protected Boolean doInBackground(byte[]... params) {
            try {
                DatagramSocket socket = new DatagramSocket();
                InetAddress IPAddress = InetAddress.getByName("255.255.255.255");
                DatagramPacket sendPacket = new DatagramPacket(params[0], params[0].length, IPAddress, 4210);
                socket.send(sendPacket);

            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
            return true;
        }
    }

}
