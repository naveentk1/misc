#!/usr/bin/env python3
"""
Simple HTTP Test Server for Octane Crawler
Creates a small website with interconnected pages for testing the crawler
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import os

class TestWebServer(BaseHTTPRequestHandler):
    
    def do_GET(self):
        """Handle GET requests"""
        
        # Define test pages
        pages = {
            '/': '''
                <!DOCTYPE html>
                <html>
                <head><title>Home Page</title></head>
                <body>
                    <h1>Welcome to Test Site</h1>
                    <p>This is a test website for the Octane Crawler.</p>
                    <nav>
                        <ul>
                            <li><a href="/page1">Page 1</a></li>
                            <li><a href="/page2">Page 2</a></li>
                            <li><a href="/about">About</a></li>
                        </ul>
                    </nav>
                </body>
                </html>
            ''',
            '/page1': '''
                <!DOCTYPE html>
                <html>
                <head><title>Page 1</title></head>
                <body>
                    <h1>Page 1</h1>
                    <p>This is the first test page.</p>
                    <nav>
                        <a href="/">Home</a> |
                        <a href="/page2">Page 2</a> |
                        <a href="/contact">Contact</a>
                    </nav>
                </body>
                </html>
            ''',
            '/page2': '''
                <!DOCTYPE html>
                <html>
                <head><title>Page 2</title></head>
                <body>
                    <h1>Page 2</h1>
                    <p>This is the second test page.</p>
                    <nav>
                        <a href="/">Home</a> |
                        <a href="/page1">Page 1</a> |
                        <a href="/services">Services</a>
                    </nav>
                </body>
                </html>
            ''',
            '/about': '''
                <!DOCTYPE html>
                <html>
                <head><title>About</title></head>
                <body>
                    <h1>About Us</h1>
                    <p>This is a test server for web crawler testing.</p>
                    <a href="/">Back to Home</a>
                </body>
                </html>
            ''',
            '/contact': '''
                <!DOCTYPE html>
                <html>
                <head><title>Contact</title></head>
                <body>
                    <h1>Contact</h1>
                    <p>Contact page for testing.</p>
                    <a href="/">Home</a> | <a href="/page1">Page 1</a>
                </body>
                </html>
            ''',
            '/services': '''
                <!DOCTYPE html>
                <html>
                <head><title>Services</title></head>
                <body>
                    <h1>Our Services</h1>
                    <p>Services page for testing.</p>
                    <a href="/">Home</a> | <a href="/page2">Page 2</a>
                </body>
                </html>
            '''
        }
        
        # Get the requested path
        path = self.path
        
        # Remove query strings if any
        if '?' in path:
            path = path.split('?')[0]
        
        # Check if page exists
        if path in pages:
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(pages[path].encode())
        else:
            # 404 Not Found
            self.send_response(404)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            html = '''
                <!DOCTYPE html>
                <html>
                <head><title>404 Not Found</title></head>
                <body>
                    <h1>404 - Page Not Found</h1>
                    <p>The page you requested does not exist.</p>
                    <a href="/">Go to Home</a>
                </body>
                </html>
            '''
            self.wfile.write(html.encode())
    
    def log_message(self, format, *args):
        """Custom logging"""
        print(f"[REQUEST] {self.address_string()} - {format % args}")

def run_server(port=80):
    """Start the test server"""
    server_address = ('', port)
    httpd = HTTPServer(server_address, TestWebServer)
    
    print("=" * 60)
    print("   Test Web Server for Octane Crawler")
    print("=" * 60)
    print(f"\nServer running on http://localhost:{port}/")
    print("\nAvailable pages:")
    print("  - http://localhost/")
    print("  - http://localhost/page1")
    print("  - http://localhost/page2")
    print("  - http://localhost/about")
    print("  - http://localhost/contact")
    print("  - http://localhost/services")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60)
    print()
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer stopped.")
        httpd.server_close()

if __name__ == '__main__':
    import sys
    
    # Check if running as root (needed for port 80)
    if os.geteuid() != 0:
        print("WARNING: Port 80 requires root privileges.")
        print("Running on port 8080 instead.")
        print("You can run with: sudo python3 test_server.py")
        print()
        run_server(8080)
    else:
        run_server(80)