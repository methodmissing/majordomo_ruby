# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')
=begin
class TestThreading < MajordomoTestCase
  def test_send_recv
    threads = []
    received = false
    threads << Thread.new do
      client = Majordomo::Client.new(BROKER, true)
      client.timeout = 100
      p client.send("thread_test", "ping")
    end

    threads << Thread.new do
      worker = Majordomo::Worker.new(BROKER, "thread_test", true)
      msg = worker.recv
      worker.recv("pong")
      received = true if msg
    end

    threads.map(&:join)
    assert received
  end
end
=end