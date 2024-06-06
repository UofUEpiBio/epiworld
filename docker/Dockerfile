FROM ubuntu:latest

# Installing g++-9 and GNU Make
RUN apt update && apt install -y --no-install-recommends g++-11
RUN apt install -y --no-install-recommends make

# Making the path availables
RUN cd /bin/ && ln -s g++-11 g++ && \
	ln -s gcov-11 gcov && \
	echo export PATH=$PATH:/bin/:/user/bin/ > ~/.profile

# Test run
COPY hello.cpp /home/hello.cpp 
RUN echo "all:\n\tg++ -std=c++11 hello.cpp -o hello.o && ./hello.o" >> /home/Makefile


CMD ["bash"]
