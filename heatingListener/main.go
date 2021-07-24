package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"mime"
	"net/http"
	"os"
	"strings"
	"time"
)

const EXIT_TIMEOUT = 5
const DEFAULT_FILE_EXT = ".txt"

type Page struct {
	Title string
	Body  []byte
	Type  string
	Time  time.Time
}

type Config struct {
	ConfigFile   string
	AppName      string
	Port         string
	StaticDir    string
	ShutDownPath string
}

var server *http.Server
var config *Config

func main() {
	if len(os.Args) < 2 {
		log.Println("FATAL: Config file name must be provided")
		os.Exit(1)
	}
	config = readConfg(os.Args[0], os.Args[1], true)

	log.Printf("INFO:  Server is listening. Port=%s\n", config.Port)
	http.HandleFunc("/", RootHandler)
	if config.ShutDownPath != "" {
		http.HandleFunc("/"+config.ShutDownPath, ShutDownHandler)
	}
	server = &http.Server{Addr: ":" + config.Port, Handler: nil}
	err := server.ListenAndServe()
	log.Printf("INFO:  Server has shutdown: %s", err)
}

func RootHandler(w http.ResponseWriter, r *http.Request) {
	log.Printf("REQ:   RootHandler. Path='%s'\n", r.URL.Path[1:])
	p, code, err := loadPage(config.StaticDir, r.URL.Path[1:])
	if err != nil {
		errorResponse(w, err.Error(), code, "resource")
	} else {
		// t, _ := template.ParseFiles("view.html")
		// t.Execute(w, p)
		setHeadings(w, p.Type, 0)
		http.ServeContent(w, r, p.Title, p.Time, bytes.NewReader(p.Body))
	}
}

func ShutDownHandler(w http.ResponseWriter, r *http.Request) {
	log.Printf("REQ:   ShutDownHandler. Timeout=%d seconds\n", EXIT_TIMEOUT)
	setHeadings(w, "application/json", 200)
	w.Write([]byte("{\"status\":\"shutting-down\"}"))
	go shutDown()
}

func shutDown() {
	ctx, cancel := context.WithTimeout(context.Background(), EXIT_TIMEOUT*time.Second)
	defer cancel()
	err := server.Shutdown(ctx)
	if err != nil {
		log.Printf("ERROR: EXIT failed. Message='%s'\n", err.Error())
	}
}

func errorResponse(w http.ResponseWriter, error string, code int, dataType string) {
	log.Printf("ERROR: Status=%d Message='%s' Datatype='%s'\n", code, error, dataType)
	setHeadings(w, "application/json", code)
	w.Write([]byte(fmt.Sprintf("{\"error\":\"%s\", \"status\":%d, \"type\":\"%s\"}", error, code, dataType)))
}

func setHeadings(w http.ResponseWriter, contentType string, code int) {
	w.Header().Add("Content-Type", contentType)
	w.Header().Add("Server", config.AppName)
	if code != 0 {
		w.WriteHeader(code)
	}
}

func loadPage(path string, title string) (*Page, int, error) {
	if strings.HasPrefix(title, string(os.PathSeparator)) || strings.Contains(title, "..") {
		return nil, 403, fmt.Errorf("forbidden path %s", title)
	}
	filename := path + title
	if !strings.ContainsRune(title, '.') {
		filename = path + title + DEFAULT_FILE_EXT
	}
	stats, err := os.Stat(filename)
	if err != nil {
		log.Println("ERROR: " + err.Error())
		return nil, 404, fmt.Errorf("not found [%s]", title)
	}
	contentType := typeFromFileName(filename)

	log.Printf("INFO:  READ FILE Title='%s' File='%s' Mime='%s' Modtime=%s\n", title, filename, contentType, stats.ModTime().Format("2006-01-02 15:04:05"))
	body, err := ioutil.ReadFile(filename)
	if err != nil {
		log.Println("ERROR: " + err.Error())
		return nil, 404, fmt.Errorf("not found [%s]", title)
	}
	return &Page{Title: title, Body: body, Type: contentType, Time: stats.ModTime()}, 0, nil
}

func typeFromFileName(filename string) string {
	i := strings.Index(filename, ".")
	if i <= 0 {
		return mime.TypeByExtension(DEFAULT_FILE_EXT)
	}
	cType := mime.TypeByExtension(filename[i:])
	if cType != "" {
		return cType
	}
	return mime.TypeByExtension(DEFAULT_FILE_EXT)
}

func readConfg(application string, fileName string, echo bool) *Config {
	file, err := ioutil.ReadFile(fileName)
	if err != nil {
		log.Printf("FATAL: Config file '%s' could not be read. %s\n", fileName, err)
		os.Exit(1)
	}
	data := Config{}
	err = json.Unmarshal([]byte(file), &data)
	if err != nil {
		log.Printf("FATAL: Config file '%s' could not be parsed. %s\n", fileName, err)
		os.Exit(1)
	}
	if data.Port == "" {
		data.Port = "8080"
	}
	if data.StaticDir == "" {
		data.StaticDir = "static/"
	}
	if data.AppName == "" {
		i := strings.LastIndex(application, "/")
		data.AppName = application[i+1:]
	}
	data.ConfigFile = fileName
	if echo {
		dataStr, err := json.Marshal(data)
		if err != nil {
			log.Fatalf(err.Error())
		}
		log.Printf("INFO:  Config%s\n", string(dataStr))
	}
	return &data
}
