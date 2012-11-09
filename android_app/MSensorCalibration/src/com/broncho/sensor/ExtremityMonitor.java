/*
 * Copyright 2009 Google Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.broncho.sensor;

/**
 * A helper class that tracks a minimum and a maximum of a variable.
 * 
 * @author Sandor Dornbush
 */
public class ExtremityMonitor {

	/**
	 * The smallest value seen so far.
	 */
	private float[] min = { 0, 0, 0 };

	/**
	 * The largest value seen so far.
	 */
	private float[] max = { 0, 0, 0 };

	public ExtremityMonitor() {
		reset();
	}

	/**
	 * Updates the min and the max with the new value.
	 * 
	 * @param value
	 *            the new value for the monitor
	 * @return true if an extremity was found
	 */
	public boolean updateX(float value) {
		boolean changed = false;
		if (value < min[0]) {
			min[0] = value;
			changed = true;
		}
		if (value > max[0]) {
			max[0] = value;
			changed = true;
		}
		return changed;
	}

	public boolean updateY(float value) {
		boolean changed = false;
		if (value < min[1]) {
			min[1] = value;
			changed = true;
		}
		
		if (value > max[1]) {
			max[1] = value;
			changed = true;
		}
		
		return changed;
	}
	
	public boolean updateZ(float value) {
		boolean changed = false;
		if (value < min[2]) {
			min[2] = value;
			changed = true;
		}
		
		if (value > max[2]) {
			max[2] = value;
			changed = true;
		}
		
		return changed;
	}
	
	/**
	 * Gets the minimum value seen.
	 * 
	 * @return The minimum value passed into the update() function
	 */
	public float getMinX() {
		return min[0];
	}
	
	public float getMinY() {
		return min[1];
	}
	
	public float getMinZ() {
		return min[2];
	}

	/**
	 * Gets the maximum value seen.
	 * 
	 * @return The maximum value passed into the update() function
	 */
	public float getMaxX() {
		return max[0];
	}
	
	public float getMaxY() {
		return max[1];
	}
	
	public float getMaxZ() {
		return max[2];
	}

	/**
	 * Resets this object to it's initial state where the min and max are
	 * unknown.
	 */
	public void reset() {
		for (int i = 0; i < 3; i++) {
			min[i] = Float.POSITIVE_INFINITY;
			max[i] = Float.NEGATIVE_INFINITY;
		}
	}

	/**
	 * Sets the minimum and maximum values.
	 */
	public void setX(float min, float max) {
		this.min[0] = min;
		this.max[0] = max;
	}
	
	public void setY(float min, float max) {
		this.min[1] = min;
		this.max[1] = max;
	}

	public void setZ(float min, float max) {
		this.min[2] = min;
		this.max[2] = max;
	}
	/**
	 * Sets the minimum value.
	 */
	public void setMinX(float min) {
		this.min[0] = min;
	}
	
	public void setMinY(float min) {
		this.min[1] = min;
	}
	
	public void setMinZ(float min) {
		this.min[2] = min;
	}

	/**
	 * Sets the maximum value.
	 */
	public void setMaxX(float max) {
		this.max[0] = max;
	}
	
	public void setMaxY(float max) {
		this.max[1] = max;
	}
	
	public void setMaxZ(float max) {
		this.max[2] = max;
	}

	public boolean hasData() {
		return min[0] != Float.POSITIVE_INFINITY && max[0] != Float.NEGATIVE_INFINITY
			&& min[1] != Float.POSITIVE_INFINITY && max[1] != Float.NEGATIVE_INFINITY
			&& min[2] != Float.POSITIVE_INFINITY && max[2] != Float.NEGATIVE_INFINITY;
	}

	@Override
	public String toString() {
		return "Min: " + min[0] + " " + min[1] + " " + min[2] + "\n" +
			   " Max: " + max[0] + " " + max[1] + " " + max[2];
	}
}
