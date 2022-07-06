# TCPCameraIP
<h2>Hardware</h2>
<ol>
<li>Beaglebone </li>
<img src="https://user-images.githubusercontent.com/57071897/177499206-c19524a8-856e-4f06-9e8e-ca5a50581896.png"></img>
<li> Webcam </li> 
<img src="https://user-images.githubusercontent.com/57071897/177499589-b503afff-8bd5-40f4-a819-8d2d36845e00.png"></img>
</ol>
<h2> Software</h2>
<pre>
sudo apt-get install -y libv4l-dev v4l2-utils make built-essentials pkg-config
</pre>
<p> Clone this repository </p>
<pre>
make dir
</pre>
<p>Check device file with v4l2-utils</p>
<pre>
v4l2-ctl --list-devices
</pre>
<p>Check pixel format which camera support</p>
<pre>
v4l2-ctl --list-formats
</pre>
<p>After check device file and pixel format then change my source code suitable your project and make to generate binary file</p>
<pre>
make
</pre>
<p>The binary file will apear in bin directory</p>
