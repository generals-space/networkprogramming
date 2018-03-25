#!/usr/bin/env python
#!encoding: utf-8

import cgi
form = cgi.FieldStorage()
print "Content-Type: text/html"
print ""
print "<html>"
print "<h2>CGI Script Output</h2>"
print "<p>"
print "The user entered data are:<br>"
print "<b>username:</b> " + form["username"].value + "<br>"
print "<b>password:</b> " + form["password"].value + "<br>"
print "</p>"
print "</html>"
