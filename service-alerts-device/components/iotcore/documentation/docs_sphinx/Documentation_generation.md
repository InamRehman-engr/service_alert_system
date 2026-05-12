
```
docker build -t docs-image:0.1 -f .\docker\docs.Dockerfile .
```

```
docker run -it -v iot-core:/app docs-image:0.1 sh -c "cd /app/documentation/docs_sphinx && make html"
```