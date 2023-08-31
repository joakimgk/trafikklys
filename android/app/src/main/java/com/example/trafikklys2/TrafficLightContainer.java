package com.example.trafikklys2;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.net.Socket;
import java.util.Stack;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class TrafficLightContainer extends FrameLayout
{
	float cellDpSize = 50;
	int cellPixelSize = 10;

	public boolean mAssigning = false;

	private TextView mClientIdLabel;

	int newPosX = 0;

	int horizntalCellCount = 10;
	int verticalCellCount = 10;

	int horizontalMargin = 0;
	int verticalMargin = 0;

	public TrafficLightContainer(@NonNull Context context)
	{
		super(context);
		init(context, null);
	}

	public TrafficLightContainer(@NonNull Context context, @Nullable AttributeSet attrs)
	{
		super(context, attrs);
		init(context, attrs);
	}

	public TrafficLightContainer(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
	{
		super(context, attrs, defStyleAttr);
		init(context, attrs);
	}

	public TrafficLightContainer(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes)
	{
		super(context, attrs, defStyleAttr, defStyleRes);
		init(context, attrs);
	}

	private void init(Context context, @Nullable AttributeSet attrs)
	{
		setCellSize(cellDpSize);
		mClientIdLabel = findViewById(R.id.client_id);
	}

	public void setCellSize(float cellSize)
	{
		cellDpSize = cellSize;
		cellPixelSize = (int) DimenUtils.calculatePixelFromDp(getContext(), cellDpSize);
		requestLayout();
	}

	public float getCellPixelSize()
	{
		return cellPixelSize;
	}

	public float getCellSize()
	{
		return cellDpSize;
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
	{
		// Here you would normally measure self and all your children, going back and forth
		// until you've decided how big this view needs to be, and how big your children should be.
		// We assume a size that doesn't care about the size of the children, and know exactly
		// what we want our children to be, so we just let the default FrameLayout do its thing here.
		super.onMeasure(widthMeasureSpec, heightMeasureSpec);

		int numCells = 0;
		for (int i = 0; i < getChildCount(); i++) {
			final TrafficLight child = ( TrafficLight ) getChildAt(i);
			if (child.getCellOrientation() % 180 == 90) numCells += 3;
			else numCells++;
		}
		int widthDp = this.getMeasuredWidth();
		setCellSize((float)widthDp / (Math.max(numCells, 7) * 3));

	}

	private void update()
	{
		// Figure out how many cells we can fit.
		// It's an into so it will automatically be rounded down
		horizntalCellCount = this.getMeasuredWidth() / cellPixelSize;
		verticalCellCount = this.getMeasuredHeight() / cellPixelSize;

		// Calculate how much extra space we have on the sides/top/bottom,
		// so we can offset everything by that and have it be centered
		horizontalMargin = ( this.getMeasuredWidth() - (horizntalCellCount * cellPixelSize ) ) / 2;
		verticalMargin = ( this.getMeasuredHeight() - (verticalCellCount * cellPixelSize ) ) / 2;

		clampAll();

	}

	private void clampAll()
	{
		for (int i = 0; i < getChildCount(); i++)
		{
			final TrafficLight child = (TrafficLight) getChildAt(i);
			child.cellX = clampX( child.cellX );
			child.cellY = clampY( child.cellY );
		}
	}

	public void  addCell()
	{
		TrafficLight newLight = new TrafficLight(getContext(), newPosX );
		newPosX += cellPixelSize;
		addView( newLight );
	}

	public static class Coordinates
	{
		public float x;
		public float y;

		public Coordinates(float x, float y)
		{
			this.x = x;
			this.y = y;
		}
	}

	// Limit to visible grid
	public int clampX(int x)
	{
		return DimenUtils.clamp(x, 0, horizntalCellCount-1);
	}

	public int clampY(int y)
	{
		return DimenUtils.clamp(y, 0, verticalCellCount-1);
	}

	public Coordinates indexFromPosition(float localX, float localY)
	{
		// position + margin ( extra space ) + half the width of a cell / the width of a cell.
		int x = (int)(( localX - horizontalMargin  + ( cellPixelSize * 0.5f ) ) / cellPixelSize);
		int y = (int)(( localY - verticalMargin + ( cellPixelSize * 0.5f ) ) / cellPixelSize);

		return new Coordinates(clampX(x), clampY(y));
	}

	// Returns left and top positions of view
	public Coordinates positionFromIndex(int indedX, int indexY)
	{
		float x = (cellPixelSize * indedX) + horizontalMargin;
		float y = (cellPixelSize * indexY) + verticalMargin;

		return new Coordinates(x,y);
	}

	@Override
	protected void onLayout(boolean changed, int left, int top, int right, int bottom)
	{
		// Skip measure and just go straight for the layout.
		// Will not work if wrap_content is used for this view, since we just fill the available space
		// and I haven't implemented that measure stuff for that.

		update();
		final int count = getChildCount();
		FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT, Gravity.CENTER);

		for (int i = 0; i < count; i++)
		{
			final TrafficLight child = ( TrafficLight ) getChildAt(i);
			if (child.getVisibility() != GONE)
			{
				Coordinates drawPosition = positionFromIndex(child.cellX, child.cellY);

				int x = (int) drawPosition.x;
				int y = (int) drawPosition.y;

				int horizontalCells = (child.getCellOrientation() % 180) == 90 ? 3 : 1;
				int verticalCells = (child.getCellOrientation() % 180) == 0 ? 3 : 1;

				int childWidth =  cellPixelSize * horizontalCells;
				int childHeight = cellPixelSize * verticalCells;

				// Normally a parent would call measure on all its children multiple times to figure out
				// what size it wants itself and its children to be. We've already decided on the size,
				// so just tell the view the size it's going to be so it can do its own layout stuff.
				child.measure(
						MeasureSpec.makeMeasureSpec( childWidth, MeasureSpec.EXACTLY ),
						MeasureSpec.makeMeasureSpec( childHeight, MeasureSpec.EXACTLY )
						);

				// Then actually position it
				// left, top, right, bottom position inside parent
				child.layout( x, y, x + childWidth, y + childHeight  );

				/*
				//addView(mClientIdLabel, params);
				TextView clientIDLabel = new TextView(getContext());
				clientIDLabel.setMinLines(1);
				clientIDLabel.setText("Hei hei");
				clientIDLabel.layout(x, y, x + childWidth + cellPixelSize, y + childHeight + cellPixelSize);
			*/
			}
		}

	}
}
