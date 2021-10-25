const fs = require('fs');
const http = require('http');

const dir = 'data/'

const requestListener = function (req, res) {
  // receive chunks of data
  let data = '';
  req.on('data', chunk => {
    data += chunk;
  })
  req.on('end', () => {
    // send response
    res.writeHead(200);
    res.end();

    if (req.headers["user-agent"] == "Arduino/1.0") {
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
    }
  })

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
server.listen(8080);
