# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestThreading < MajordomoTestCase
  def test_send_recv
    threads = []
    req_received, rep_received = false, false
    threads << Thread.new do
      client = Majordomo::Client.new(BROKER, true)
      client.timeout = 100
      client.send("thread_test", "ping")
      reply = client.recv("thread_test")
      rep_received = true if reply
    end

    threads << Thread.new do
      worker = Majordomo::Worker.new(BROKER, "thread_test", true)
      request, reply_to = worker.recv
      req_received = true if request
      worker.send("pong", reply_to)
    end

    threads.map(&:join)
    assert req_received
    assert rep_received
  end
end
