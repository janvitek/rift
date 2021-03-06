#!/usr/bin/env ruby

require 'fileutils'

def usage
    puts "usage: ./export.rb version dir"
    exit 1
end

usage if ARGV.size != 2

HOME = File.dirname(__FILE__)

FILES = ['CMakeLists.txt', 'LICENSE', 'local', 'src', 'README.md', 'tests',
         '.gitignore', 'assignments']
IGNORE = [/.*swp/]

EXPORT_DIR = ARGV[1]
VERSION = Integer(ARGV[0])

if Dir.exists? EXPORT_DIR
    puts "#{EXPORT_DIR} already exists. Overwriting? (y/n)"
    res = STDIN.gets.strip
    unless res == "y"
        puts "abort"
        exit 1
    end
    FILES.each do |f|
        trg = File.join(EXPORT_DIR, f)
        FileUtils.rm_rf(trg)
    end
end

FileUtils.mkdir_p(EXPORT_DIR)

def crawl(f)
    src = File.join(HOME, f)
    trg = File.join(EXPORT_DIR, f)

    IGNORE.each do |i|
        return if i =~ src
    end
    begin
        if File.directory?(src)
            FileUtils.mkdir_p(trg)
            Dir.foreach(src) do |g|
                next if g == '.' or g == '..'
                crawl(File.join(f,g))
            end
        else
            output = ""
            print = [true]
            File.readlines(src).each do |line|
                if line =~ /#if (VERSION (<=|>=|<|>|==) \d+)/
                    print << (eval($1) && print.last)
                elsif line =~ /#endif .*VERSION/
                    if (print.size == 1)
                        puts "Unbalanced endif in #{src}"
                        exit 1
                    end
                    print.pop
                else
                    output << line if print.last
                end
            end
            if (print.size != 1)
                puts "Unclosed endif in #{src}"
                exit 1
            end
            File.open(trg, 'w') { |file| file.write(output) } if output != ""
        end
    rescue
        puts "Error in File #{f}"
        exit 1
    end
end

FILES.each do |f|
    puts "Cannot export #{f}: does not exist" unless File.exists? f
    crawl f
end

File.open(File.join(EXPORT_DIR, '.version'), 'w') { |file| file.write(VERSION) }
