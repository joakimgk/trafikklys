package com.example.trafikklys2;

import android.content.Context;
import android.util.TypedValue;

/**
 * Created by Roughy on 6/28/2017.
 */

public class DimenUtils
{
	public static float calculatePixelFromDp(Context context, float dpSize)
	{
		return TypedValue.applyDimension( TypedValue.COMPLEX_UNIT_DIP, dpSize, context.getResources().getDisplayMetrics() );
	}

	// Can you believe there isn't a stock clamp method?
	public static int clamp(int val, int min, int max)
	{
		if (val > max)
			val = max;
		if (val < min)
			val = min;

		return val;
	}

}
