package main

import (
	"gopkg.in/qml.v0"
	"math/rand"
	"time"
)

func main() {
	qml.Init(nil)
	engine := qml.NewEngine()
	component, err := engine.LoadFile("share/%ProjectName%/Main.qml")
	if err != nil {
		panic(err)
	}

	ctrl := Control{Message: "Hello from Go!"}

	context := engine.Context()
	context.SetVar("ctrl", &ctrl)

	window := component.CreateWindow(nil)

	ctrl.Root = window.Root()

	rand.Seed(time.Now().Unix())

	window.Show()
	window.Wait()
}

type Control struct {
	Root    qml.Object
	Message string
}

func (ctrl *Control) Hello() {
    go func() {
            ctrl.Message = "Hello from Go Again!"
            qml.Changed(ctrl, &ctrl.Message)
    }()
}
