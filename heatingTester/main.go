package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"unicode"
)

const ORD_OF_ZERO = 48
const SECS_1_HR = 60
const MINS_1_DAY = 1440

func main() {

	var argsWithoutProg = os.Args[1:]
	var fileName string
	var schedule []int = make([]int, 0, 50)
	var schedCount = 0
	var hostName string
	var action string
	var isGet bool = true

	fmt.Println("ARGS :", argsWithoutProg)

	for i, a := range argsWithoutProg {
		if strings.HasPrefix(strings.ToLower(a), "file:") {
			fileName = a[5:]
		} else {
			if strings.HasPrefix(strings.ToLower(a), "host:") {
				hostName = a[5:]
			} else {
				if strings.HasPrefix(strings.ToLower(a), "get:") {
					action = a[4:]
					isGet = true
				} else {
					if strings.HasPrefix(strings.ToLower(a), "post:") {
						action = a[5:]
						isGet = false
					} else {
						schedule = append(schedule, parseTimeToMinutes(a, i))
						schedCount++
					}
				}
			}
		}
	}

	if hostName != "" {
		fmt.Println("HOST :", hostName)
		if action == "" {
			exitWithError("No action for host was given", 1)
		}
		if isGet {
			if schedCount > 0 {
				fmt.Println(scheduleToString(schedule, schedCount))
			}
			fmt.Println("GET  :", action)
			body, err := sendGet(hostName, action)
			if err != nil {
				exitWithError(err.Error(), 1)
			} else {
				fmt.Println(body)
			}
		} else {
			fmt.Println("POST :", action)
			body, err := sendPost(hostName, action, scheduleToString(schedule, schedCount))
			if err != nil {
				exitWithError(err.Error(), 1)
			} else {
				fmt.Println(body)
			}
		}
	}

	if fileName != "" {
		fmt.Println("FILE :", fileName)
		fmt.Println("ITEMS:", schedCount)
		writeScheduleDataToFile(fileName, schedule, schedCount)
	}

}

func sendGet(host string, path string) (string, error) {
	resp, err := http.Get(host + "/" + path)
	if err != nil {
		return "", err
	}
	if (resp.StatusCode / 100) != 2 {
		return "", fmt.Errorf("status code (%d). message '%s'. host '%s/%s'", resp.StatusCode, resp.Status, host, path)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}
	return string(body), nil
}

func sendPost(host string, path string, jsonContent string) (string, error) {
	resp, err := http.Post(host+"/"+path, "application/json", strings.NewReader(jsonContent))
	if err != nil {
		return "", err
	}
	if (resp.StatusCode / 100) != 2 {
		body, _ := ioutil.ReadAll(resp.Body)
		return "", fmt.Errorf("status code (%d). message '%s' body '%s'. host '%s/%s'", resp.StatusCode, resp.Status, body, host, path)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}
	return string(body), nil
}

func writeScheduleDataToFile(fileName string, schedule []int, schedCount int) {
	if schedCount == 0 {
		exitWithError("No schedule times given", 1)
	}
	file, err := os.Create(fileName)
	if err != nil {
		exitWithError(err.Error(), 1)
	}
	defer file.Close()

	file.WriteString(scheduleToString(schedule, schedCount))
}

func scheduleToString(schedule []int, schedCount int) string {
	var buff strings.Builder
	buff.WriteString("[")
	for day := 0; day < 7; day++ {
		for i, m := range schedule {
			fmt.Printf("D[%d] M[%d] = %d\n", day, i, (day*MINS_1_DAY)+m)
			buff.WriteString(fmt.Sprintf("%d", (day*MINS_1_DAY)+m))
			if i < (schedCount - 1) {
				buff.WriteString(",")
			}
		}
		if day < 6 {
			buff.WriteString(",")
		}
	}
	buff.WriteString("]")
	return buff.String()
}

func parseTimeToMinutes(s string, i int) int {
	h := 0
	m := 0
	hrs := true
	for _, c := range s {
		if (c == ':') || (c == '.') {
			hrs = false
		} else {
			if unicode.IsDigit(c) {
				if hrs {
					h = (h * 10) + (int(c) - ORD_OF_ZERO)
				} else {
					m = (m * 10) + (int(c) - ORD_OF_ZERO)
				}
			} else {
				exitWithError(fmt.Sprintf("Invalid digit: Parameter[%d]: '%s'", i, s), 1)
			}
		}
	}
	if h > 23 {
		exitWithError(fmt.Sprintf("Hour is > 23: Parameter[%d]: '%s'", i, s), 1)
	}
	if m > 59 {
		exitWithError(fmt.Sprintf("Minute is > 59: Parameter[%d]: '%s'", i, s), 1)
	}
	return (h * SECS_1_HR) + m
}

func exitWithError(s string, rc int) {
	fmt.Printf("EXIT Code(%d). ERROR: %s\n", rc, s)
	os.Exit(rc)
}
