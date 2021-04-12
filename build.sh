mkdir -p dist
docker build -t filtergrapher .
docker create -ti --name filtergrapher-container filtergrapher
docker cp filtergrapher-container:/build/dist/ www
docker rm -fv filtergrapher-container