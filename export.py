import os
import socket
import bisect
import struct
import argparse

class Registry(object):

	def registry(self):
		return ('afrinic', 'apnic', 'arin', 'iana', 'lacnic', 'ripencc')

	def __init__(self, name):
		self.name = name

	def __repr__(self):
		return 'Registry(%s)' % self.name


class Version(object):

	def __init__(self, version, registry, serial, records, startdate, enddate, UTCoffset):
		self.version = version
		self.registry = registry
		self.serial = serial
		self.records = records
		self.startdate = startdate
		self.enddate = enddate
		self.UTCoffset = UTCoffset

	def __repr__(self):
		return 'Version(%s,%s,%s)' % (self.version, self.registry, self.serial)

class Summary(object):

	def __init__(self, registry, _a, type, _b, count, summary):
		self.registry = registry
		self.type = type
		self.count = count
		self.summary = summary

	def __repr__(self):
		return 'Summary(%s,%s,%s)' % (self.registry, self.type, self.count)

class Record(object):

	@classmethod
	def Type(cls):
		return ('asn', 'ipv4', 'ipv6')

	def __init__(self, registry, cc, type, start, value, date, status, extensions=None):
		self.registry = registry
		self.cc = cc
		self.type = type
		self.start = start
		self.value = value
		self.date = date
		self.status = status
		self.extensions = extensions

	@property
	def addr(self):
		if self.type == 'ipv4':
			return socket.inet_pton(socket.AF_INET, self.start)
		elif self.type == 'ipv6':
			return socket.inet_pton(socket.AF_INET6, self.start)

	def __cmp__(self, other):
		return cmp(self.addr, other.addr)

	def __repr__(self):
		return 'Record(%s,%s,%s,%s,%s)' % (self.registry, self.cc, self.type, self.start, self.value)

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--cc', help='filter country code', action='append')
	parser.add_argument('--db', help='output binary database filename', default='ipcountry.db')
	parser.add_argument('--csv', help='output csv filename', default='ipcountry.csv')
	parser.add_argument('--rir', help='RIR directory', default='RIR')
	parser.add_argument('--type', help='ipv4 or ipv6')
	args = parser.parse_args()

	records = []
	for name in os.listdir(args.rir):
		filename = os.path.join(args.rir, name)

		with open(filename) as file:
			version = None
			summary = []

			for line in file:
				# ignore blank line
				if not line.strip():
					continue
				# ignore comment
				if line.strip().startswith('#'):
					continue
				if not version:
					version = Version(*line.split('|'))
					continue

				if len(summary) < len(Record.Type()):
					summary.append(Summary(*line.split('|')))
					continue

				record = Record(*line.split('|'))
				records.append(record)
	if args.cc:
		records = filter(lambda x: x.cc in args.cc, records)

	ipv4records = []
	if args.type is None or args.type == 'ipv4':
		ipv4records = filter(lambda x: x.type == 'ipv4', records)
		ipv4records.sort()
		print 'ipv4: %d' % len(ipv4records)

	ipv6records = []
	if args.type is None or args.type == 'ipv6':
		ipv6records = filter(lambda x: x.type == 'ipv6', records)
		ipv6records.sort()
		print 'ipv6: %d' % len(ipv6records)

	with open(args.db, 'w') as db:
		db.write(struct.pack('>L', len(ipv4records)))
		db.write(struct.pack('>L', len(ipv6records)))

		for ip in ipv4records:
			db.write(struct.pack('>4sLL4s', ip.addr, int(ip.value), 0, ip.cc))

		for ip in ipv6records:
			db.write(struct.pack('>16sLLL4s', ip.addr, int(ip.value), 0, 0, ip.cc))
	with open(args.csv, 'w') as csv:
		csv.write('start,value,type,cc\n')

		for ip in ipv4records:
			csv.write('%s,%s,%s,%s\n' % (ip.start, ip.value, ip.type, ip.cc))

		for ip in ipv6records:
			csv.write('%s,%s,%s,%s\n' % (ip.start, ip.value, ip.type, ip.cc))

if __name__ == '__main__':
	main()
