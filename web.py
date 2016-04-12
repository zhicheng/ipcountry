import tornado.web
import tornado.ioloop

import json
from ipcountry import IPCountry

ipcountry = IPCountry('ipcountry.db')

class WebHandler(tornado.web.RequestHandler):

	def get(self):
		ip = self.get_argument('ip', self.request.remote_ip).strip()
		cc = ipcountry.lookup(ip)
		self.render('web.html', ip=ip, cc=cc)

class APIHandler(tornado.web.RequestHandler):

	def get(self):
		ip = self.get_argument('ip', self.request.remote_ip).strip()
		cc = ipcountry.lookup(ip)
		self.set_header('Content-Type', 'application/javascript')

		data = {'cc': cc, 'ip': ip}
		callback = self.get_argument('callback', None)
		if callback:
			self.finish('%s(%s);' % (callback, json.dumps(data)))
		else:
			self.finish(data)

def make_app():
	return tornado.web.Application([
		(r"/", WebHandler),
		(r"/api", APIHandler),
	], debug=True, xheaders=True)

if __name__ == "__main__":
	tornado.log.enable_pretty_logging()
	app = make_app()
	app.listen(9000)
	tornado.ioloop.IOLoop.current().start()
