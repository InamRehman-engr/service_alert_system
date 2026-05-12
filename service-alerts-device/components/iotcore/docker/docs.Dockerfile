FROM python:3.10.5-slim-buster
RUN apt-get update
RUN apt-get install doxygen python3-sphinx clang -y
RUN pip install sphinx-rtd-theme breathe sphinx-sitemap myst_parser hawkmoth clang sphinx-copybutton
WORKDIR /app