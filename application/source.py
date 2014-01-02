import socket

# Create socket. 
s = socket.socket(); 

# Bind to the port
s.bind(('', 7236))        

# Now wait for client connection.
s.listen(5)                 

# Accept any connection. 
conn, addr = s.accept(); 

# Send data
conn.send('test')

# Close connection
conn.close(); 
