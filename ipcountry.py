#!/usr/bin/env python

import os
import struct
import socket
import bisect

class IPCountryRecord(object):

	def __init__(self, start, value, type, cc):
		self.start = start
		self.value = value
		self.type = type
		self.cc = cc

	def __cmp__(self, other):
		return cmp(self.number, other.number)

	@property
	def number(self):
		if self.type == 'ipv4':
			return struct.unpack('>L', self.start)[0]
		elif self.type == 'ipv6':
			return struct.unpack('>4L', self.start)[0]

class IPCountry(object):

	def __init__(self, filename):
		self.filename = filename

		with open(filename, 'r') as db:
			self.ipv4len = struct.unpack('>L', db.read(4))[0]
			self.ipv6len = struct.unpack('>L', db.read(4))[0]

			self.ipv4records = []
			for i in range(self.ipv4len):
				start, value, flags, cc = struct.unpack('>4sLL4s', db.read(16))
				self.ipv4records.append(IPCountryRecord(start, value, 'ipv4', cc[:2]))

			self.ipv6records = []
			for i in range(self.ipv6len):
				start, value, flags, unuse, cc = struct.unpack('>16sLLL4s', db.read(32))
				self.ipv6records.append(IPCountryRecord(start, value, 'ipv6', cc[:2]))


	def lookup(self, addr):
		res = socket.getaddrinfo(addr, None, socket.AF_UNSPEC,
		                         socket.SOCK_STREAM, 0, socket.AI_NUMERICHOST)
		
		if not res:
			return None

		family = res[0][0]
		if family == socket.AF_INET:
			type = 'ipv4'
		elif family == socket.AF_INET6:
			type = 'ipv6'
		else:
			return None

		key = IPCountryRecord(socket.inet_pton(family, addr), 0, type, '')

		if family == socket.AF_INET:
			records = self.ipv4records
		elif family == socket.AF_INET6:
			records = self.ipv6records

		i = bisect.bisect_right(records, key) - 1
		if i < 0:
			return None

		record = records[i]
		if record > key:
			return None

		value = record.value if family == socket.AF_INET else (1 << (128 - record.value))
		if record.number + value < key.number:
			return None 

		return record.cc
