const fs = require('fs');
const path = require("path");
const http = require('http');

const port = 8080;
const dir = 'data/';
let data = '';

const getMostRecentFile = (dir) => {
  const files = orderReccentFiles(dir);
  return files.length ? files[0] : undefined;
};

const orderReccentFiles = (dir) => {
  return fs.readdirSync(dir)
    .filter((file) => fs.lstatSync(path.join(dir, file)).isFile())
    .map((file) => ({ file, mtime: fs.lstatSync(path.join(dir, file)).mtime }))
    .sort((a, b) => b.mtime.getTime() - a.mtime.getTime());
};

const requestListener = function (req, res) {
  req.on('error', (err) => {
    console.error(err);
    res.statusCode = 400;
    res.end();
  });
  res.on('error', (err) => {
    console.error(err);
  });

  if (req.method === 'POST' && req.url === '/data') {
    // && req.headers["user-agent"] == "Arduino/1.0") {
    // receive chunks of data
    data = '';
    req.on('data', chunk => {
      data += chunk;
    })
    req.on('end', () => {
      // send response
      res.writeHead(200);
      res.end();

      // get csv data and received date as last line
      let csv_data = '';
      let file_date = 'output';
      if (data.lastIndexOf("\n") > 0) {
        lastNewLineIdx = data.lastIndexOf("\n")
        csv_data = data.substring(0, lastNewLineIdx);
        file_date = data.substring(lastNewLineIdx + 1, data.length);
      } else {
        csv_data = data;
      }

      if (!fs.existsSync(dir)){
        fs.mkdirSync(dir);
      }
      // write to file
      fs.writeFile(dir + file_date + '.csv', csv_data, function (err) {
        if (err) return console.log(err);
      });

      // log to console
      console.log(csv_data);
      console.log();
      console.log(Date());
    })
  }
  else if (req.method === 'GET' && req.url === '/') {
    let lastFile = getMostRecentFile(dir);
    if (lastFile !== undefined) {
      fs.readFile(dir + lastFile.file, (err, data) => {
        if (!err) {
          res.writeHead(200);
          res.end(lastFile.mtime + '\n\n' + data);
        }
      })
    }
    else {
      res.writeHead(200);
      res.end("No file");
    }
  }
  else if (req.method === 'GET' && req.url === '/download') {
    let lastFile = getMostRecentFile(dir);
    if (lastFile !== undefined) {
      // create read steam for the pdf
      const rs = fs.createReadStream(dir + lastFile.file);
      res.setHeader("Content-Disposition", "attachment; filename=" + lastFile.file);
      rs.pipe(res);
    }
    else {
      res.writeHead(200);
      res.end("No file");
    }
  }
  else {
    res.statusCode = 404;
    res.end("404: Not found");
  }

  // print http orignal message like curl
  // request.method, URL and httpVersion
  console.log(req.method + ' ' + req.url + ' HTTP/' + req.httpVersion);

  // request.headers
  for (var property in req.headers) {
      if (req.headers.hasOwnProperty(property)) {
          console.log(property + ': ' + req.headers[property])
      }
  }
  console.log()
}

const server = http.createServer(requestListener);
server.listen(port);
