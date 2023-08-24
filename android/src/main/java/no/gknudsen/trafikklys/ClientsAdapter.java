package no.gknudsen.trafikklys;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.LayerDrawable;
import android.support.v7.widget.RecyclerView;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;

import java.util.ArrayList;


public class ClientsAdapter extends RecyclerView.Adapter<ClientsAdapter.MyViewHolder> {

    private LayoutInflater inflater;
    private Context context;

    public ClientsAdapter(Context context, ArrayList<Client> clients) {
        inflater = LayoutInflater.from(context);
        this.context = context;
    }

    @Override
    public int getItemViewType(int position) {
        return Utility.clients.get(position).rotation;
    }

    public MyViewHolder onCreateViewHolder(final ViewGroup parent, int viewType) {
        View view;
        if (viewType == 0) view = inflater.inflate(R.layout.client, parent, false);
        else view = inflater.inflate(R.layout.client_horizontal, parent, false);

        return new MyViewHolder(view);
    }

    @Override
    public void onBindViewHolder(MyViewHolder holder, int position) {

        // Converts 14 dip into its equivalent px
        float dip = 64f * 3 + 2*16f;
        Resources r = context.getResources();
        float px = TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dip,
                r.getDisplayMetrics()
        );

        int h = holder.mm.getLayoutParams().height;
        holder.mm.setRotation(90);
        holder.mm.getLayoutParams().width = (int)px;
        //holder.mm.getLayoutParams().height -= 700;

        //holder.mm.requestLayout();
    }

    @Override
    public int getItemCount() {
        return Utility.clients.size();
    }

    class MyViewHolder extends RecyclerView.ViewHolder
    {
        public LinearLayout mm;

        public MyViewHolder(View itemView) {
            super(itemView);
            mm = itemView.findViewById(R.id.samba);

        }

        public View getView() {
            itemView.getParent().requestLayout();
            return itemView;
        }
    }
}

/*
// https://guides.codepath.com/android/Using-an-ArrayAdapter-with-ListView
public class ClientsAdapter extends ArrayAdapter<Client> {

    public ClientsAdapter(Context context, ArrayList<Client> clients) {
        super(context, 0, clients);
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        Client client = getItem(position);
        if (convertView == null) {
            convertView = LayoutInflater.from(getContext()).inflate(R.layout.client, parent, false);
        }
        convertView.setRotation(client.rotation);
        return convertView;
    }
}
*/
