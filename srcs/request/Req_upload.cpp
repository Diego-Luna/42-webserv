#include "Req.hpp"

void	Req::parseUpload(void)
{
								cout << "start of parsing upload" << endl;
	
	string	boundary = findUploadBoundry();
					cout << "boundary: |" << boundary << "|" << endl;

					// cout << "boundary: |" << boundary << "|" << endl;

	if (_error == false)
		_fileName = findUploadFileName(boundary);

					cout << "fileName: |" << _fileName << "|" << endl;

	if (_error == false)
		_body = findUploadBody(boundary);

					cout << "body: |" << _body << "|" << endl;

	_populateEnv(string("Host"));
	_populateEnv(string("User-Agent"));
	_populateEnv(string("Content-Length"));
	_populateEnv(string("Accept"));
	_populateEnv(string("Accept-Language"));
	_populateEnv(string("Connection"));
	_buildEncoded();
	env["CONTENT_TYPE"] = getContentType(_extension);
	env["SERVER_PROTOCOL"] = _protocol;
	env["FILE_NAME"] = _fileName;

	set_status_code(OK);
}

void	Req::createUploadFile()
{
	FILE	*uploadFile = fopen(_fileName.c_str(), "w");

	if (uploadFile == NULL)
	{
		std::cerr << "error. could not create file: " << _fileName << endl;

		_error = true;
		set_status_code(INTERNAL_SERVER_ERROR);
		return;
	}
	_uploadFiles.push_back(_fileName);

	fwrite(_body.c_str(), sizeof(char), _body.length(), uploadFile);
	fclose(uploadFile);
}

	// picks up the stream from where findUploadFileName left off
string	Req::findUploadBody(string boundary)
{
	boundary = trimLine(boundary);
	boundary = "--" + boundary + "--";

	string line;
	string body = "";
	
	std::getline(_ReqStream, line);
	line = trimLine(line);
	while (!line.empty())
	{
		std::getline(_ReqStream, line);
		line = trimLine(line);
	}
					// cout << "after first loop: " << line << endl;
					// cout << "boundary: |" << boundary << "|" << endl;
					// cout << line.compare(0, boundary.length(), boundary) << endl;

		// working, but could use some cleanup.
	while (!_ReqStream.eof() && line.compare(0, boundary.length(), boundary) != 0)
	{
		std::getline(_ReqStream, line);
		line = trimLine(line);
		if (line.compare(0, boundary.length(), boundary) == 0) {
			body.pop_back();		// removes extraneous \n
			break;
		}
		body += line + "\n";
	}
	return body;
}

string	Req::trimLine(string line)
{
	while (line.back() == '\n' || line.back() == '\r')
		line.pop_back();
	return line;
}

string	Req::findUploadBoundry(void)
{
	string boundary = "boundary=";
	if (_http_Req.find(boundary) == string::npos)
	{
								cout << "BOUNDRY NOT FOUND" << endl;
		_error = true;
		set_status_code(BAD_REQUEST);
		return "";
	}
	string::iterator it = _http_Req.begin() + _http_Req.find(boundary) + boundary.length();
	boundary = "";
	while (*it != '\n' && *it != '\r' && *it != ' ')
	{
		boundary += *it;
		it++;
	}

	return boundary;
}
		// KNOWN BUG: filename can't have a space in it.

string	Req::findUploadFileName(string boundary)
{
	string	fileName = "";
	string	line;

		// Reset the stream to the beginning
	_ReqStream.clear();            // Clear any error flags
	_ReqStream.seekg(0, _ReqStream.beg); // Set the position to the beginning

	boundary = "--" + boundary;
	
	std::getline(_ReqStream, line);
	while (!line.empty() && line.compare(0, boundary.length(), boundary) != 0)
		std::getline(_ReqStream, line);
	std::getline(_ReqStream, line);

				cout << "line after boundary?\n" << line << endl;
	
	string	tmp = "Content-Disposition:";
	while (!line.empty() && line.compare(0, tmp.length(), tmp) != 0)
		std::getline(_ReqStream, line);
	if (line.find("filename=") == string::npos)
	{
		_error = true;
		set_status_code(BAD_REQUEST);
		return "";
	}
	string::iterator it = line.begin() + line.find("filename=") + 9;
	while (it != line.end() && *it != ' ')
	{
		fileName += *it;
		it++;
	}
	fileName = trimLine(fileName);
	stripQuotes(fileName);
	fileName = PATH_TO_UPLOAD + fileName;
	return fileName;
}

void	Req::stripQuotes(string &original)
{
	if (original.front() == '"')
		original.erase(0, 1);
	if (original.back() == '"')
		original.pop_back();
	return;
}