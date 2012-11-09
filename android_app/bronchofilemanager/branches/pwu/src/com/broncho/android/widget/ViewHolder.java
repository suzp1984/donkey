
package com.broncho.android.widget;

public class ViewHolder<T> {
    public T data;

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((data == null) ? 0 : data.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;

        ViewHolder other = (ViewHolder)obj;
        if (data == null) {
            if (other.data != null)
                return false;
        } else if (!data.equals(other.data)) {
            return false;
        }

        return true;
    }
}
