# RiskyBird Academy

This is the tutorial created by [Dima Nikiforov](https://www.linkedin.com/in/dima) and [Kris Dong](https://www.linkedin.com/in/chris-dong/) for RiskyBird Academy. The material has been taught at the University of California, Berkeley, for entry-level high school students who want to learn following concepts:

* Introduction to Embedded Systems
* Machine Learning Programming on Edge device 
* Introduction to Robotics
* Scientific Programming and Quantization
* Deep Neural Networks (DNN)
* Reinforcement Learning (RL)
  
The slides of the class are available through the following link: [lecture slides](https://docs.google.com/presentation/d/1aLcF6UbZxzHJhVKkxmO-jFUWJreh792BKoxEXGfzUTs/)

The material has been made open source FOR NON-COMMERCIAL USE only. Please contact the author for any questions: <vnikiforov@berkeley.edu>, <krisdong@berkeley.edu>

## Basic Installation

Users of this course are recommended to install the following software packages. The packages have been tested on Windows 10 and above, Mac OSX, and Ubuntu Linux systems.

* Python 3.8 via Miniconda (conda): <https://docs.conda.io/en/latest/miniconda.html>
* Git: <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>
* Visual Studio Code (code): <https://code.visualstudio.com/download>

**NOTE: If you are running conda and code in Windows, launching code from Anaconda Prompt will make sure that the conda environment available inside the integrated Terminal window. Otherwise you may encounter error that conda is not installed**

Once Python 3.8 is installed, the following Python modules can be installed using pip within the conda environment (for example, in (base) environment):
~~~
    python -m pip install numpy matplotlib
~~~

## Tensorflow Installation for Windows and Linux

For the DNN portion, we use tensorflow 2 and keras. The installation shall reference the official documentation: <https://www.tensorflow.org/install>. 

**Due to the fact that Google stopped supporing Windows Native environment above tensorflow 2.10, please make sure to install and use tensorflow 2.10 for compatibility with our tested code**

**If your PC comes with supported NVidia GPU for accelerating DNN code, please follow carefully the setup of GPU support for Linux and Windows Native. For MacOS, please read the instruction at the end**

After you have setup your system for an installation on either CPU or GPU, run the following pip script to install tensorflow 2.10
~~~
    pip install --upgrade pip
    pip install tensorflow==2.10
    pip install numpy==1.21
~~~

Finally, you may verify tensorflow has been properly set up by running the following test
~~~
    python -c "import tensorflow as tf; print(tf.reduce_sum(tf.random.normal([1000, 1000])))"
~~~

If you have install tensorflow with GPU support, you may verify by running the following test
~~~
    python -c "import tensorflow as tf; print(tf.config.list_physical_devices('GPU'))"
~~~

## Tensorflow Installation with Metal accelerated backend for MacOS

Apple has released a Metal-accelerated tensorflow backend to utilize the GPU on MacOS. Please visit Apple website to install tensorflow 2.10: <https://developer.apple.com/metal/tensorflow-plugin/>

## Installation of Reinforcement Learning Modules

This course will require installation of the following reinforcement learning modules:

* gym simulator: <https://github.com/openai/gym>
~~~
 pip install gym==0.17.3
 pip install pyglet==1.5.11
~~~

* gym-pybullet-drones simulator
~~~
 git submodule update --init --recursive
 cd sim/gym-pybullet-drones/
 pip3 install --upgrade pip
 pip3 install -e .
 pip3 install pyserial
~~~

