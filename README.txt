==================================================
Compiling and testing the webserver with no cache. 
==================================================

1) make cleanall
This clears all the previous stale builds

2) Edit the makefile and replace CFLAGS with this line
    CFLAGS = -std=c++11 -Wall -g -O3   -DHTTP_V11
3) make
This builds the web server.

====================================================
Compiling and testing the webserver with 100% cache. 
====================================================
1) Modify servercache.h file such that the macro
    #define CACHE_SIZE 2000
   is greater than or equal to the number of HTML files

2) make cleanall
To clear previous build

3) make
Build the server

====================================================
Compiling and testing the webserver with 50% cache. 
====================================================
1) Modify servercache.h file such that the macro
    #define CACHE_SIZE 1000
   is greater than or equal to the number of HTML files

2) make cleanall
To clear previous build

3) make
Build the server

====================================================
Apache 2 configuration
====================================================

1) cp scripts/apache2.conf /etc/apache2/
    Update the config directory to point to code 
    directory in your machine.
    <Directory /home/siu/sem2/ece670/project/code>
    	Options Indexes FollowSymLinks
    	AllowOverride None
    	Require all granted
    </Directory>

2) cp scripts/000-default.conf /etc/apache2/sites-enabled
   
   Update the below line to point to html folder 
   in your machine.

	DocumentRoot /home/siu/sem2/ece670/project/code/html

==================================================
Testing the webserver
==================================================
1) make gen
This generates 2000 random html files in html folder

2) make run
This starts the web server application with working
directory as $PWD/html and port as 9000

3) make test
This runs httperf wrapper script to generate
result log files in output directory.

4) make test SERVER=<IP address of server>
To generate results from remote machine.
The client is running in different machine.

5) make test SERVER=<IP address of server> PORT=80
To generate results from remote machine for apache2
