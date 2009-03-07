#!/usr/bin/python

"""
Classes to make a continuous video stream playlist from an RSS feed.
The script is planned to be a useful tool for video online databases as
http://giss.tv/dmmdb and plumi.org based sites to be able to do 
continuity tv channels on the internet. 

The script takes a an argument the URL of the RSS feed. If it's not specified will
fetch http://giss.tv/dmmdb/rss.php?channel=piksel08 by default.

In order to run the script you need RSS.py, and rss parser you can found here::
http://www.mnot.net/python/RSS.py

The script is still not functional as we need new features on the freej core and 
in the freej python bindings. But it's an example as a simple user case and as 
feature request need example ;)

"""

import threading
import freej
import time
import sys
import inspect
import urllib
from RSS import ns, CollectionChannel, TrackingChannel


class RSSsucker(object):
    """A RSSsucker is a RSS parser wich returns a video URL list of a RSS1.0 feed."""

    def __init__(self, url="http://giss.tv/dmmdb/rss.php?channel=piksel08"):
        self.url = url
        self.suck_feed()

    def suck_feed(self):
        #Indexes RSS data by item URL
        tc = TrackingChannel()

        #Returns the RSSParser instance used, which can usually be ignored
	print "Fetching ",self.url," ..."
        tc.parse(self.url)

        RSS10_TITLE = (ns.rss10, 'title')
        RSS10_DESC = (ns.rss10, 'description')

        self.list = list() 

        items = tc.listItems()
        for item in items:
            url = item[0].replace("ol","dl")
            print url
            self.list.append(url)
	

class RSSPlaylist(object):
  """A RSSPlaylist is a running freej context and a video playlist from an RSS."""

  def __init__(self, width=320, height=240, url = "http://giss.tv/dmmdb/rss.php?channel=piksel08"):
    self.width = width
    self.height = height
    self.rsssucker = RSSsucker(url)
    self.ctx_thread = None

    if len(self.rsssucker.list):
      self.cx = freej.Context()
      self.cx.init(self.width, self.height, 0, 0)
      self.cx.plugger.refresh(self.cx)
      self.ctx_thread = threading.Thread(target = self.cx.start,
                                         name = "freej")
      self.ctx_thread.start()

      while (True):
        for file in self.rsssucker.list:
	  print file
	  self.lay = self.cx.open(urllib.pathname2url(file).replace("%3A",":"))
	  self.lay.fit()
	  self.lay.active = True
	  self.cx.add_layer(self.lay)
	  time.sleep(5)
          self.cx.rem_layer(self.lay)
    else:
      print "Cannot start a show without playlist!"







if len(sys.argv) > 1:
    rssplaylist = RSSPlaylist(320,240,sys.argv[1])
else:
    rssplaylist = RSSPlaylist()
