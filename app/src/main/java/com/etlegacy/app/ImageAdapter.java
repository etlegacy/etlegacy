package com.etlegacy.app;

import android.content.Context;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import androidx.recyclerview.widget.RecyclerView;

import org.libsdl.app.SDLActivity;

public class ImageAdapter extends RecyclerView.Adapter<ImageAdapter.ImageViewHolder> {
	private final Context context;
	private final Integer[] images;
	private final RecyclerView recyclerView;

	public ImageAdapter(Context context, Integer[] images, RecyclerView recyclerView) {
		this.context = context;
		this.images = images;
		this.recyclerView = recyclerView;
	}

	@Override
	public ImageViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
		ImageView imageView = new ImageView(parent.getContext());
		imageView.setLayoutParams(new ViewGroup.LayoutParams(100, 100));
		imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
		imageView.setPadding(8, 8, 8, 8);
		return new ImageViewHolder(imageView);
	}

	@Override
	public void onBindViewHolder(ImageViewHolder holder, int position) {
		holder.imageView.setImageResource(images[position]);
		holder.imageView.setOnClickListener(v -> {
			switch (position) {
				case 0:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_1);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_1);
					break;
				case 1:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_2);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_2);
					break;
				case 2:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_3);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_3);
					break;
				case 3:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_4);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_4);
					break;
				case 4:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_5);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_5);
					break;
				case 5:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_6);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_6);
					break;
				case 6:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_7);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_7);
					break;
				case 7:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_8);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_8);
					break;
				case 8:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_9);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_9);
					break;
				case 9:
					SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_0);
					SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_0);
					break;
			}
			recyclerView.setVisibility(View.GONE);
		});
	}

	@Override
	public int getItemCount() {
		return images.length;
	}

	static class ImageViewHolder extends RecyclerView.ViewHolder {
		ImageView imageView;

		ImageViewHolder(ImageView itemView) {
			super(itemView);
			imageView = itemView;
		}
	}
}
