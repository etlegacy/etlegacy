package com.karin.idTech4Amm.network;

import android.annotation.SuppressLint;

import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.lib.ThreadUtility;

import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

@SuppressLint({"TrustAllX509TrustManager", "CustomX509TrustManager", "BadHostnameVerifier"})
public final class NetworkAccessManager
{
    public static final int CONST_TIME_OUT = 60000;

    public static void AsyncGet(String urlStr, Runnable callback, String[] args)
    {
        ThreadUtility.Post("NETWORK", new Runnable()
        {
            @Override
            public void run()
            {
                args[0] = Get(urlStr);
            }
        }, new Runnable()
        {
            @Override
            public void run()
            {
                callback.run();
            }
        });
    }

    public static String Get(String urlStr)
    {
        HttpURLConnection conn = null;
        InputStream inputStream = null;

        try
        {
            URL url = new URL(urlStr);
            conn = (HttpURLConnection)url.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(CONST_TIME_OUT);
            conn.setInstanceFollowRedirects(true);
            if(conn instanceof HttpsURLConnection)
                SkipSSLVerification((HttpsURLConnection)conn);
            conn.setDoInput(true); // 总是读取结果
            conn.setUseCaches(false);
            conn.connect();

            int respCode = conn.getResponseCode();
            if(respCode == HttpURLConnection.HTTP_OK)
            {
                inputStream = conn.getInputStream();
                byte[] data = FileUtility.ReadStream(inputStream);
                if(null != data && data.length > 0)
                {
                    return new String(data);
                }
                else
                    return "";
            }
            else
                return null;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return null;
        }
        finally {
            FileUtility.CloseStream(inputStream);
        }
    }

    private static void SkipSSLVerification(HttpsURLConnection conn)
    {
        try
        {
            SSLContext sc = SSLContext.getInstance("TLS");
            sc.init(null, new TrustManager[]{
                    new X509TrustManager() {
                        @Override
                        public X509Certificate[] getAcceptedIssuers() {
                            return new X509Certificate[]{};
                        }
                        @Override
                        public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException
                        { }
                        @Override
                        public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException { }
                    }
            }, new java.security.SecureRandom());
            SSLSocketFactory newFactory = sc.getSocketFactory();
            conn.setSSLSocketFactory(newFactory);
            conn.setHostnameVerifier(new HostnameVerifier() {
                @Override
                public boolean verify(String hostname, SSLSession session) {
                    return true;
                }
            });
        }
        catch (Exception e)
        {
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }
}
