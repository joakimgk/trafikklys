package com.example.trafikklys2;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

import static com.example.trafikklys2.MainActivity.GRN;
import static com.example.trafikklys2.MainActivity.NON;
import static com.example.trafikklys2.MainActivity.RED;
import static com.example.trafikklys2.MainActivity.YLW;

public class Animate {
    private static byte[] Reverse(byte[] p) {
        int len = p.length;
        byte[] rev = new byte[len];
        for (int i = 0; i < len; i++) {
            rev[len - i - 1] = p[i];
        }
        return rev;
    }

    private static byte[] TRAIL = {
        RED, YLW, GRN
    };

    private static byte[] replace(byte[] dd, byte[] val, int startPos) {
        byte[] newArray = new byte[dd.length];
        for (int i = 0; i < dd.length; i++) {
            newArray[i] = i >= startPos && i - startPos < val.length ? val[i - startPos] : dd[i];
        }
        return newArray;
    }

    private static boolean isReversed(Client c) {
        return c.mTrafficLight.getCellOrientation() == 180 || c.mTrafficLight.getCellOrientation() == 270;
    }

    private static boolean isHorizontal(Client c) {
        return (c.mTrafficLight.getCellOrientation() % 180) == 90;
    }

    private static boolean isVertical(Client c) {
        return (c.mTrafficLight.getCellOrientation() % 180) == 0;
    }

    private static int countVerticalMiddles() {
        int count = 0;
        for (int i = 0; i < Utility.clients.size(); i++) {
            if (i == 0 || i == Utility.clients.size() -1) continue;
            Client c = Utility.clients.get(i);
            if (isVertical(c)) {
                count++;
            }
        }
        return count;
    }

    private static byte[] concat(byte[] a, byte[] b) {
        int len = a.length + b.length;
        byte[] newArray = new byte[len];
        for (int i = 0; i < len; i++) {
            newArray[i] = i < a.length ? a[i] : b[i - a.length];
        }
        return newArray;
    }

    public static ArrayList<Program> Trail() {
        Collections.sort(Utility.clients, new ClientsComparator());
        int verticalMiddles = countVerticalMiddles();

        int len = Utility.clients.size() + verticalMiddles;
        ArrayList<Program> p = new ArrayList<>(len);

        // fill all with empty programs, of correct length
        byte[] emptyBytes = new byte[len * 3];
        Arrays.fill(emptyBytes, NON);

        int pos = 0;
        for (int i = 0; i < Utility.clients.size(); i++) {
            Client client = Utility.clients.get(i);
            byte[] prog = isReversed(client) ? Reverse(TRAIL) : TRAIL;

            if (i > 0 && i < Utility.clients.size() && isVertical(client)) {
                // add extra step
                Program tmp = new Program(replace(
                        emptyBytes,
                        concat(Reverse(prog), prog),
                        pos * 3
                ), client);
                pos += 2;
                p.add(tmp);
            } else {
                Program tmp = new Program(replace(
                        emptyBytes,
                        prog,
                        pos * 3
                ), client);
                pos++;
                p.add(tmp);
            }
        }
        return p;
    }

}
