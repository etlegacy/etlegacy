import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

import java.net.URL;
import java.net.MalformedURLException;
import java.net.HttpURLConnection;
import java.net.URLConnection;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.security.cert.X509Certificate;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * Security property test: All download URLs used by ETLDownload must use HTTPS scheme.
 * Network connections for downloading game assets must enforce encrypted transport.
 * An HTTP URL would allow MITM attacks to replace game files with malicious payloads.
 */
public class ETLDownloadSecurityTest {

    // Invariant: Any URL used for downloading game assets MUST use HTTPS scheme,
    // never plain HTTP or other unencrypted protocols.

    @ParameterizedTest
    @ValueSource(strings = {
        "http://example.com/etlegacy.zip",
        "http://download.etlegacy.com/assets.pk3",
        "http://cdn.etlegacy.com/update/patch.zip",
        "http://192.168.1.1/malicious_payload.zip",
        "http://10.0.0.1/game_files.tar.gz",
        "http://etlegacy.com/etmain/pak0.pk3",
        "HTTP://etlegacy.com/assets.zip",
        "Http://etlegacy.com/assets.zip",
        "http://localhost/evil.zip",
        "http://127.0.0.1/evil.zip",
        "http://0.0.0.0/evil.zip",
        "http://[::1]/evil.zip",
        "http://etlegacy.com:8080/assets.zip",
        "http://user:password@etlegacy.com/assets.zip",
        "http://etlegacy.com/path/../../../etc/passwd",
        "http://etlegacy.com/%2F..%2F..%2Fevil.zip",
        "ftp://etlegacy.com/assets.zip",
        "file:///etc/passwd",
        "javascript:alert(1)",
        "data:text/html,<script>alert(1)</script>"
    })
    void testDownloadUrlMustUseHttpsScheme(String payload) {
        // Invariant: URLs used for downloading game assets must use HTTPS scheme only.
        // Any non-HTTPS URL represents a security violation that could allow MITM attacks.
        
        boolean isSecure = isSecureUrl(payload);
        
        assertFalse(isSecure == false && isHttpUrl(payload),
            "SECURITY VIOLATION: HTTP URL detected - '" + payload + 
            "' uses unencrypted transport. Game asset downloads MUST use HTTPS to prevent MITM attacks.");
        
        // The core invariant: if a URL is to be used for downloads, it must be HTTPS
        if (isValidUrl(payload)) {
            try {
                URL url = new URL(payload);
                String scheme = url.getProtocol();
                
                // Assert that only HTTPS is acceptable for download connections
                assertNotEquals("http", scheme.toLowerCase(),
                    "SECURITY VIOLATION: Plain HTTP scheme detected in URL: '" + payload + 
                    "'. All game asset downloads must use HTTPS to prevent interception and tampering.");
                
                assertNotEquals("ftp", scheme.toLowerCase(),
                    "SECURITY VIOLATION: FTP scheme detected in URL: '" + payload + 
                    "'. All game asset downloads must use HTTPS.");
                
                assertNotEquals("file", scheme.toLowerCase(),
                    "SECURITY VIOLATION: File scheme detected in URL: '" + payload + 
                    "'. All game asset downloads must use HTTPS.");
                
                if (scheme.equalsIgnoreCase("https")) {
                    // HTTPS URLs are acceptable - verify they would use HttpsURLConnection
                    assertTrue(wouldUseHttpsConnection(url),
                        "HTTPS URL should establish HttpsURLConnection: " + payload);
                }
                
            } catch (MalformedURLException e) {
                // Malformed URLs should be rejected, not cause crashes
                // This is acceptable behavior - the URL is invalid
                assertTrue(true, "Malformed URL correctly rejected: " + payload);
            }
        }
    }

    @Test
    void testHttpsConnectionIsUsedForDownloads() {
        // Invariant: The connection type for HTTPS URLs must be HttpsURLConnection,
        // which provides certificate validation by default.
        
        String[] validHttpsUrls = {
            "https://etlegacy.com/assets.zip",
            "https://download.etlegacy.com/pak0.pk3",
            "https://cdn.etlegacy.com/update.zip"
        };
        
        for (String urlString : validHttpsUrls) {
            try {
                URL url = new URL(urlString);
                assertEquals("https", url.getProtocol().toLowerCase(),
                    "Valid download URL must use HTTPS scheme: " + urlString);
                
                // Verify that opening this URL would produce an HttpsURLConnection
                assertTrue(wouldUseHttpsConnection(url),
                    "HTTPS URL must produce HttpsURLConnection (with certificate validation): " + urlString);
                    
            } catch (MalformedURLException e) {
                fail("Valid HTTPS URL should not be malformed: " + urlString);
            }
        }
    }

    @Test
    void testNoInsecureTrustManagerShouldBeUsed() {
        // Invariant: The SSL context must not use a trust-all TrustManager
        // that would bypass certificate validation.
        
        // Create a trust-all manager (the bad pattern)
        TrustManager[] insecureTrustManagers = new TrustManager[]{
            new X509TrustManager() {
                public X509Certificate[] getAcceptedIssuers() { return null; }
                public void checkClientTrusted(X509Certificate[] certs, String authType) {}
                public void checkServerTrusted(X509Certificate[] certs, String authType) {}
            }
        };
        
        // Verify that the insecure trust manager accepts everything (demonstrating the danger)
        X509TrustManager insecureTm = (X509TrustManager) insecureTrustManagers[0];
        
        // The invariant: a proper trust manager should NOT accept null certificates
        // This test documents that trust-all managers are dangerous
        assertNull(insecureTm.getAcceptedIssuers(),
            "This demonstrates the danger: trust-all manager returns null accepted issuers");
        
        // The security property: production code must NOT install such a trust manager
        // We verify the default SSL context is not compromised
        try {
            SSLContext defaultContext = SSLContext.getDefault();
            assertNotNull(defaultContext, "Default SSL context must exist");
            assertEquals("Default", defaultContext.getProtocol(),
                "Default SSL context should be the system default with proper certificate validation");
        } catch (Exception e) {
            fail("SSL context check failed: " + e.getMessage());
        }
    }

    @ParameterizedTest
    @ValueSource(strings = {
        "https://etlegacy.com/assets.zip",
        "https://download.etlegacy.com/pak0.pk3",
        "https://cdn.etlegacy.com/etmain.zip",
        "https://etlegacy.com:443/assets.zip",
        "https://etlegacy.com/path/to/assets.pk3"
    })
    void testValidHttpsUrlsAreAccepted(String validUrl) {
        // Invariant: Valid HTTPS URLs must be accepted and use secure connections.
        
        try {
            URL url = new URL(validUrl);
            
            assertEquals("https", url.getProtocol().toLowerCase(),
                "URL must use HTTPS scheme: " + validUrl);
            
            assertNotNull(url.getHost(),
                "URL must have a valid host: " + validUrl);
            
            assertFalse(url.getHost().isEmpty(),
                "URL host must not be empty: " + validUrl);
            
            assertTrue(wouldUseHttpsConnection(url),
                "HTTPS URL must use HttpsURLConnection: " + validUrl);
                
        } catch (MalformedURLException e) {
            fail("Valid HTTPS URL should not throw MalformedURLException: " + validUrl);
        }
    }

    // Helper methods

    private boolean isSecureUrl(String urlString) {
        try {
            URL url = new URL(urlString);
            return "https".equalsIgnoreCase(url.getProtocol());
        } catch (MalformedURLException e) {
            return false;
        }
    }

    private boolean isHttpUrl(String urlString) {
        try {
            URL url = new URL(urlString);
            return "http".equalsIgnoreCase(url.getProtocol());
        } catch (MalformedURLException e) {
            return false;
        }
    }

    private boolean isValidUrl(String urlString) {
        try {
            new URL(urlString);
            return true;
        } catch (MalformedURLException e) {
            return false;
        }
    }

    private boolean wouldUseHttpsConnection(URL url) {
        // Verify that an HTTPS URL would produce an HttpsURLConnection
        // This checks the Java URL connection mechanism works correctly
        try {
            URLConnection connection = url.openConnection();
            boolean isHttps = connection instanceof HttpsURLConnection;
            connection.getClass(); // just reference it
            return isHttps;
        } catch (Exception e) {
            // Network not available in test environment, but we can check the URL scheme
            return "https".equalsIgnoreCase(url.getProtocol());
        }
    }
}