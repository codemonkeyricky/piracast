import re
import socket

m1_options_rsp = 'RTSP/1.0 200 OK\r\n' \
          'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n' \
          'Server: stagefright/1.2 (Linux;Android 4.3)\r\n' \
          'CSeq: 1\r\n' \
          'Public: org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER\r\n' \
          '\r\n'

m2_req =    'OPTIONS * RTSP/1.0\r\n' \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n' \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n' \
            'CSeq: 2\r\n' \
            'Require: org.wfa.wfd1.0\r\n' \
            '\r\n'

m3_body =   'wfd_video_formats: 39 00 02 02 0001FFFF 3FFFFFFF 00000000 00 0000 0000 00 none none\r\n'   \
            'wfd_audio_codecs: LPCM 00000002 00\r\n'         \
            'wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n'

m3_rsp =    'RTSP/1.0 200 OK\r\n'                               \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n'          \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n'   \
            'CSeq: 2\r\n'                                       \
            'Content-Type: text/parameters\r\n'                 \
            'Content-Length: ' + str(len(m3_body)) + '\r\n'     \
            '\r\n' + m3_body

m4_rsp =    'RTSP/1.0 200 OK\r\n'                               \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n'          \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n'   \
            'CSeq: 3\r\n'                                       \
            '\r\n'

m5_rsp =    'RTSP/1.0 200 OK\r\n'                               \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n'          \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n'   \
            'CSeq: 4\r\n'                                       \
            '\r\n'

m6_req =    'SETUP rtsp://{}/wfd1.0/streamid=0 RTSP/1.0\r\n'                    \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n'                      \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n'               \
            'CSeq: 4\r\n'                                                   \
            'Transport: RTP/AVP/UDP;unicast;client_port=50000-50001\r\n'    \
            '\r\n'

m7_req =    'PLAY rtsp://{}/wfd1.0/streamid=0 RTSP/1.0\r\n'       \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n'                      \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n'               \
            'CSeq: 5\r\n'                                                   \
            'Session: {}\r\n'                                       \
            '\r\n'

m16_rsp =   'RTSP/1.0 200 OK\r\n'                               \
            'Date: Sun, 11 Aug 2013 04:41:40 +000\r\n'          \
            'Server: stagefright/1.2 (Linux;Android 4.3)\r\n'   \
            'CSeq: {}\r\n'                                       \
            '\r\n'

def source_connect (ip) :

    # Create a socket object
    s = socket.socket()         
    
    # Connect to port. 
    s.connect((ip, 7236))
    
    # Connect via wireless interface.
    s.setsockopt(socket.SOL_SOCKET, 25, 'wlan0')
    
    # Wait to receive data
    data = s.recv(1024)
    
    print 'Received', repr(data)
    
    # Send m1 response
    s.send(m1_options_rsp)
    
    # Send m2 request
    s.send(m2_req)
    
    # Receive m2 response
    m2_rsp = s.recv(1024)
    
    print 'M2 Resp:', repr(m2_rsp)
    
    # Receive: M3 GET_PARAMETER Request
    m3_req = s.recv(1024)
    
    print 'M3 Req:\n', repr(m3_req), '\n'
    
    print 'M3 Resp:\n', repr(m3_rsp), '\n'
    
    # Send: M3 Response
    s.send(m3_rsp)
    
    # Receive: M4 RTSP SET_PARAMETER Request
    m4_req = s.recv(1024)
    
    print 'M4 Req:\n', repr(m4_req), '\n'
    
    print 'M4 Resp:\n', repr(m4_rsp), '\n'
    
    # Send: M4 response
    s.send(m4_rsp)
    
    # Receive: M5 RTSP SET_PARAMETER Request (setup)
    m5_req = s.recv(1024)
    
    # Send: M5 Response 
    s.send(m5_rsp)
    
    # Send: M6 RTSP SETUP
    s.send(m6_req.format(ip))
    
    # Receive: M6 RTSP SETUP response
    m6_rsp = s.recv(1024)
    
    print 'M6 Rsp:', repr(m6_rsp)
    
    # TODO: extract session ID
    match = re.search(r'Session: (\d*);', m6_rsp)
    
    # Send: M6 Request
    s.send(m7_req.format(ip, match.group(1)))
    
    m7_rsp = s.recv(1024)
    
    print 'M7 Rsp:', repr(m7_rsp)
    
    cseq = 5

    keep_alive_sent = 0
    
    while True :
        req = s.recv(1024)
        
        print 'req: ', req

        if req.__len__() == 0 : 

            print 'socket closed!'
            break; 
        
        if 'TEARDOWN' in req : 
            print 'Tear down received!'
            break; 

        if keep_alive_sent == 120 : 
            print 'demo times up!'
            break; 

        s.send(m16_rsp.format(cseq)); 

        print 'sent keep alive!'

        cseq += 1

        keep_alive_sent += 1
    
    s.close()
